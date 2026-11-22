// io.js — the site I/O page and forcing
//
// "Saturn PLC standard v1": the PLC exposes its I/O and force registers as
// Modbus holding registers, and the site's Home Assistant reads them as
// sensors named  sensor.<prefix>_...  (see the site's HA package).
// Saturn Command reads those sensors and forces points by calling HA's
// modbus.write_register service.
//
//   PLC address      Holding reg   HA sensor (prefix p)          Meaning
//   %MW48            1072          sensor.p_plc_inputs            real inputs (bits)
//   %MW57            1081          sensor.p_plc_inputs_effective  inputs after forces
//   %MW49            1073          sensor.p_plc_outputs           real outputs (after forces)
//   %MW58            1082          sensor.p_plc_outputs_logic     what the logic wants
//   %MW55 / %MW56    1079 / 1080   sensor.p_force_active / _remaining
//   %MW70 / %MW71    1094 / 1095   sensor.p_ai0_raw / p_ai1_raw
//   %MW60..%MW67     1084..1091    (written) DI mask, DI value, DQ mask, DQ value,
//                                  AI mask, AI0 raw, AI1 raw, auto-clear minutes

const db = require('./db');
const ha = require('./ha');

const FORCE_BASE_REGISTER = 1084;
const FORCE_MINUTES = 60;
const KINDS = ['DI', 'DQ', 'AI'];

// ---- I/O point list: one point per line ------------------------------------
//   DI0  C1-AUTO  Compressor 1 HOA in Auto
//   DQ0  C1-RUN   Compressor 1 run command
//   AI0  AIR-PT   Air pressure | bar | 0-16 | sensor.shed_air_pressure
function parsePoints(text) {
  const points = [];
  const errors = [];
  const seen = new Set();
  String(text || '').split('\n').forEach((raw, i) => {
    const line = raw.trim();
    if (!line || line.startsWith('#')) return;
    const m = line.match(/^(DI|DQ|AI)\s*(\d{1,2})\s+(\S+)\s*(.*)$/i);
    if (!m) { errors.push(`Line ${i + 1}: start with DI, DQ or AI then the channel number, e.g. "DI0 C1-AUTO Compressor 1 in Auto"`); return; }
    const kind = m[1].toUpperCase();
    const ch = parseInt(m[2], 10);
    if ((kind === 'DI' || kind === 'DQ') && ch > 15) { errors.push(`Line ${i + 1}: ${kind} channel must be 0-15`); return; }
    if (kind === 'AI' && ch > 3) { errors.push(`Line ${i + 1}: AI channel must be 0-3`); return; }
    const key = kind + ch;
    if (seen.has(key)) { errors.push(`Line ${i + 1}: ${key} is listed twice`); return; }
    seen.add(key);
    const point = { kind, ch, tag: m[3], desc: '' };
    if (kind === 'AI') {
      const parts = m[4].split('|').map(s => s.trim());
      point.desc = parts[0] || '';
      point.unit = parts[1] || '';
      const range = (parts[2] || '').match(/^(-?[\d.]+)\s*-\s*(-?[\d.]+)$/);
      point.min = range ? parseFloat(range[1]) : 0;
      point.max = range ? parseFloat(range[2]) : 100;
      if (parts[3] && /^[a-z_]+\.[a-z0-9_]+$/i.test(parts[3])) point.entity = parts[3];
      const rawRange = (parts[4] || '').match(/^raw\s*(-?\d+)\s*-\s*(-?\d+)$/i);
      point.raw4 = rawRange ? parseInt(rawRange[1], 10) : 0;
      point.raw20 = rawRange ? parseInt(rawRange[2], 10) : 10000;
    } else {
      point.desc = m[4].trim();
    }
    points.push(point);
  });
  const order = { DI: 0, DQ: 1, AI: 2 };
  points.sort((a, b) => order[a.kind] - order[b.kind] || a.ch - b.ch);
  return { points, errors };
}

function pointsToText(points) {
  return (points || []).map(p => {
    const head = `${p.kind}${p.ch}`.padEnd(5) + ' ' + String(p.tag).padEnd(10) + ' ';
    if (p.kind !== 'AI') return head + (p.desc || '');
    const parts = [p.desc || '', p.unit || '', `${p.min}-${p.max}`, p.entity || ''];
    if (p.raw4 !== 0 || p.raw20 !== 10000) parts.push(`raw ${p.raw4}-${p.raw20}`);
    while (parts.length > 1 && !parts[parts.length - 1]) parts.pop();
    return head + parts.join(' | ');
  }).join('\n');
}

const bit = (word, n) => word == null ? null : ((word >> n) & 1) === 1;
function num(states, id) {
  const s = states.get(id);
  if (!s) return null;
  const v = Number(s.state);
  return Number.isFinite(v) ? v : null;
}
function engToRaw(p, value) {
  const span = (p.max - p.min) || 1;
  const raw = Math.round(p.raw4 + ((value - p.min) / span) * (p.raw20 - p.raw4));
  return Math.max(-32768, Math.min(32767, raw));
}
function rawToEng(p, raw) {
  if (raw == null) return null;
  const span = (p.raw20 - p.raw4) || 1;
  return p.min + ((raw - p.raw4) / span) * (p.max - p.min);
}

function forcingConfigured(site) {
  return !!(site.io_prefix && site.io_hub && site.ha_url && site.ha_token_enc);
}

async function activeForces(site) {
  // Forces older than the auto-clear time are gone in the PLC too.
  if (site.force_changed_at && Date.now() - new Date(site.force_changed_at).getTime() > FORCE_MINUTES * 60000) {
    await db.query('DELETE FROM site_forces WHERE site_id = $1', [site.id]);
  }
  const r = await db.query(
    `SELECT f.kind, f.channel, f.value, f.eng_value, f.forced_at, u.name AS forced_by
       FROM site_forces f LEFT JOIN users u ON u.id = f.forced_by
      WHERE f.site_id = $1`, [site.id]);
  return r.rows;
}

// Everything the I/O page needs.
async function readIO(site) {
  const points = Array.isArray(site.io_points) ? site.io_points : [];
  const out = { configured: forcingConfigured(site), points: [], force: { active: false, remaining_s: 0, count: 0 }, ok: true };
  let forces = await activeForces(site);

  let states = null;
  if (site.ha_url && site.ha_token_enc && site.io_prefix) {
    try { states = await ha.fetchStates(site); } catch (err) { out.ok = false; out.error = err.message; }
  } else {
    out.ok = false;
    out.error = 'I/O page not connected yet: set the Home Assistant details and the I/O prefix and hub in Settings.';
  }

  const p = site.io_prefix;
  const g = states ? (name) => num(states, `sensor.${p}_${name}`) : () => null;
  const diRaw = g('plc_inputs'), diEff = g('plc_inputs_effective');
  const dqOut = g('plc_outputs'), dqLogic = g('plc_outputs_logic');
  const plcForceActive = g('force_active');
  if (states) {
    out.force.remaining_s = g('force_remaining') || 0;
    if (diRaw == null && dqOut == null) {
      out.ok = false;
      out.error = `Home Assistant has no sensor.${p}_plc_inputs / sensor.${p}_plc_outputs. Check the I/O prefix in Settings.`;
    }
  }

  // PLC already cleared its forces (timed out or restarted): forget ours too.
  if (forces.length && plcForceActive === 0 && site.force_changed_at &&
      Date.now() - new Date(site.force_changed_at).getTime() > 90000) {
    await db.query('DELETE FROM site_forces WHERE site_id = $1', [site.id]);
    await db.query('INSERT INTO site_log (site_id, user_id, entry) VALUES ($1, NULL, $2)',
      [site.id, 'All I/O forces cleared by the PLC (timed out or PLC restarted)']);
    forces = [];
  }

  const forceMap = new Map(forces.map(f => [f.kind + f.channel, f]));
  for (const pt of points) {
    const f = forceMap.get(pt.kind + pt.ch) || null;
    const row = { ...pt, forced: f ? { value: f.value, eng_value: f.eng_value, by: f.forced_by, at: f.forced_at } : null };
    if (pt.kind === 'DI') {
      row.actual = bit(diRaw, pt.ch);
      row.effective = bit(diEff, pt.ch);
    } else if (pt.kind === 'DQ') {
      row.actual = bit(dqOut, pt.ch);
      row.logic = bit(dqLogic, pt.ch);
    } else {
      const eng = pt.entity && states ? num(states, pt.entity) : null;
      const raw = pt.ch <= 1 ? g(`ai${pt.ch}_raw`) : null;
      row.raw = raw;
      row.actual = raw != null ? rawToEng(pt, raw) : null;
      row.effective = eng;              // the PLC's scaled value (after forces) if HA exposes it
      row.forceable = pt.ch <= 1;
    }
    out.points.push(row);
  }
  out.force.count = forces.length;
  out.force.active = forces.length > 0 || plcForceActive === 1;
  out.updated_at = new Date().toISOString();
  return out;
}

// Write the whole force block to the PLC from what's in the database.
async function pushForces(site, client) {
  const r = await client.query('SELECT kind, channel, value FROM site_forces WHERE site_id = $1', [site.id]);
  let diMask = 0, diVal = 0, dqMask = 0, dqVal = 0, aiMask = 0, ai0 = 0, ai1 = 0;
  for (const f of r.rows) {
    const b = 1 << f.channel;
    if (f.kind === 'DI') { diMask |= b; if (f.value) diVal |= b; }
    if (f.kind === 'DQ') { dqMask |= b; if (f.value) dqVal |= b; }
    if (f.kind === 'AI') { aiMask |= b; if (f.channel === 0) ai0 = f.value; else ai1 = f.value; }
  }
  const u16 = v => (v < 0 ? v + 65536 : v);
  await ha.callService(site, 'modbus', 'write_register', {
    hub: site.io_hub,
    slave: site.io_slave || 1,
    address: FORCE_BASE_REGISTER,
    value: [diMask, diVal, dqMask, dqVal, aiMask, u16(ai0), u16(ai1), FORCE_MINUTES],
  });
  return r.rows.length;
}

// mode: 'auto' (release), 'on', 'off', or 'value' (AI, engineering units)
async function setForce(site, user, { kind, ch, mode, value }) {
  if (!forcingConfigured(site)) throw Object.assign(new Error('Forcing is not set up for this site yet (Settings → I/O).'), { status: 400, expose: true });
  kind = String(kind || '').toUpperCase();
  ch = parseInt(ch, 10);
  const point = (site.io_points || []).find(p => p.kind === kind && p.ch === ch);
  if (!point) throw Object.assign(new Error('That point is not in this site\'s I/O list'), { status: 400, expose: true });
  if (kind === 'AI' && ch > 1) throw Object.assign(new Error('Only AI0 and AI1 can be forced'), { status: 400, expose: true });

  let rawValue = null, engText = null, describe;
  const name = `${kind}${ch} ${point.tag}`;
  if (mode === 'auto') {
    describe = `Released force on ${name}`;
  } else if (kind === 'AI') {
    const v = Number(value);
    if (mode !== 'value' || !Number.isFinite(v)) throw Object.assign(new Error('Enter a number to force the analog to'), { status: 400, expose: true });
    rawValue = engToRaw(point, v);
    engText = `${v}${point.unit ? ' ' + point.unit : ''}`;
    describe = `FORCED ${name} to ${engText}`;
  } else if (mode === 'on' || mode === 'off') {
    rawValue = mode === 'on' ? 1 : 0;
    engText = mode.toUpperCase();
    describe = `FORCED ${name} ${engText}`;
  } else {
    throw Object.assign(new Error('Unknown force mode'), { status: 400, expose: true });
  }

  const client = await db.pool.connect();
  try {
    await client.query('BEGIN');
    if (mode === 'auto') {
      await client.query('DELETE FROM site_forces WHERE site_id = $1 AND kind = $2 AND channel = $3', [site.id, kind, ch]);
    } else {
      await client.query(
        `INSERT INTO site_forces (site_id, kind, channel, value, eng_value, forced_by, forced_at)
         VALUES ($1, $2, $3, $4, $5, $6, NOW())
         ON CONFLICT (site_id, kind, channel) DO UPDATE
           SET value = EXCLUDED.value, eng_value = EXCLUDED.eng_value, forced_by = EXCLUDED.forced_by, forced_at = NOW()`,
        [site.id, kind, ch, rawValue, engText, user.id]);
    }
    await client.query('UPDATE sites SET force_changed_at = NOW() WHERE id = $1', [site.id]);
    const count = await pushForces(site, client);   // throws if HA refuses -> rollback
    await client.query('INSERT INTO site_log (site_id, user_id, entry) VALUES ($1, $2, $3)',
      [site.id, user.id, describe + (count ? ` (${count} force${count === 1 ? '' : 's'} active, auto-clear in ${FORCE_MINUTES} min)` : ' (no forces active)')]);
    await client.query('COMMIT');
  } catch (err) {
    await client.query('ROLLBACK');
    throw Object.assign(err, { status: err.status || 502, expose: true });
  } finally {
    client.release();
  }
}

async function clearAll(site, user) {
  if (!forcingConfigured(site)) throw Object.assign(new Error('Forcing is not set up for this site.'), { status: 400, expose: true });
  const client = await db.pool.connect();
  try {
    await client.query('BEGIN');
    await client.query('DELETE FROM site_forces WHERE site_id = $1', [site.id]);
    await client.query('UPDATE sites SET force_changed_at = NOW() WHERE id = $1', [site.id]);
    await pushForces(site, client);
    await client.query('INSERT INTO site_log (site_id, user_id, entry) VALUES ($1, $2, $3)', [site.id, user.id, 'Cleared ALL I/O forces']);
    await client.query('COMMIT');
  } catch (err) {
    await client.query('ROLLBACK');
    throw Object.assign(err, { status: err.status || 502, expose: true });
  } finally {
    client.release();
  }
}

module.exports = { parsePoints, pointsToText, readIO, setForce, clearAll, activeForces, FORCE_MINUTES, KINDS };
