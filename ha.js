// ha.js — reading live data from a site's Home Assistant
//
// Each site can store a Home Assistant URL (e.g. its Nabu Casa address) and a
// long-lived access token. The token is encrypted in the database with
// AES-256-GCM and never sent to the browser.

const crypto = require('crypto');

function keyOrNull() {
  const secret = process.env.DATA_KEY || process.env.SESSION_SECRET;
  if (!secret) return null;
  return crypto.createHash('sha256').update('saturn-command:' + secret).digest();
}

function encrypt(plain) {
  const key = keyOrNull();
  if (!key) throw new Error('Set SESSION_SECRET (or DATA_KEY) in Railway before saving Home Assistant tokens.');
  const iv = crypto.randomBytes(12);
  const cipher = crypto.createCipheriv('aes-256-gcm', key, iv);
  const enc = Buffer.concat([cipher.update(String(plain), 'utf8'), cipher.final()]);
  const tag = cipher.getAuthTag();
  return [iv, tag, enc].map(b => b.toString('base64')).join('.');
}

function decrypt(blob) {
  const key = keyOrNull();
  if (!key || !blob) return null;
  try {
    const [iv, tag, enc] = blob.split('.').map(s => Buffer.from(s, 'base64'));
    const decipher = crypto.createDecipheriv('aes-256-gcm', key, iv);
    decipher.setAuthTag(tag);
    return Buffer.concat([decipher.update(enc), decipher.final()]).toString('utf8');
  } catch {
    return null; // key changed or data corrupt
  }
}

function cleanUrl(url) {
  const u = String(url || '').trim().replace(/\/+$/, '');
  if (!u) return null;
  if (!/^https?:\/\//i.test(u)) throw new Error('Home Assistant URL must start with https:// (or http://)');
  return u;
}

function entityList(text) {
  return String(text || '')
    .split(/[\s,]+/)
    .map(s => s.trim())
    .filter(s => /^[a-z_]+\.[a-z0-9_]+$/i.test(s));
}

// Returns { ok, items: [{entity_id, name, state, unit, last_changed}], error }
async function fetchLive(site) {
  if (!site.ha_url || !site.ha_token_enc) {
    return { ok: false, error: 'Live data not set up for this site yet (add the Home Assistant URL and token in Settings).' };
  }
  const token = decrypt(site.ha_token_enc);
  if (!token) return { ok: false, error: 'Stored Home Assistant token could not be read. Re-enter it in Settings.' };
  const wanted = entityList(site.ha_entities);
  if (!wanted.length) {
    return { ok: false, error: 'No live points chosen yet. List the Home Assistant entity IDs to show in Settings.' };
  }
  try {
    const res = await fetch(site.ha_url + '/api/states', {
      headers: { Authorization: 'Bearer ' + token, 'Content-Type': 'application/json' },
      signal: AbortSignal.timeout(8000),
    });
    if (res.status === 401) return { ok: false, error: 'Home Assistant rejected the token (401). Check it in Settings.' };
    if (!res.ok) return { ok: false, error: `Home Assistant returned HTTP ${res.status}` };
    const all = await res.json();
    const byId = new Map(all.map(s => [s.entity_id, s]));
    const items = wanted.map(id => {
      const s = byId.get(id);
      if (!s) return { entity_id: id, name: id, state: 'not found', unit: '', last_changed: null };
      return {
        entity_id: id,
        name: (s.attributes && s.attributes.friendly_name) || id,
        state: s.state,
        unit: (s.attributes && s.attributes.unit_of_measurement) || '',
        last_changed: s.last_changed || null,
      };
    });
    return { ok: true, items, fetched_at: new Date().toISOString() };
  } catch (err) {
    const msg = err && err.name === 'TimeoutError' ? 'Home Assistant did not answer within 8 seconds' : (err.message || 'request failed');
    return { ok: false, error: 'Could not reach Home Assistant: ' + msg };
  }
}

module.exports = { encrypt, decrypt, cleanUrl, entityList, fetchLive };
