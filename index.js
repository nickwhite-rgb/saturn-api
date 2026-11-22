// index.js — Saturn Command Center
//
// One place to see and look after every site: Nick's own sites, Saturn's
// depot and customer jobs. Mobile-first web app, PIN login, per-employee
// site access (view / edit), live Home Assistant data and "Ask Claude".
//
// Endpoints (all JSON, all need a login unless noted):
//   POST /api/login, POST /api/logout, GET /api/me
//   GET  /api/sites                 sites this user can see
//   POST /api/sites                 add a site (admin or "can add sites")
//   GET  /api/sites/:id             one site (view access)
//   PATCH /api/sites/:id            edit a site (edit access)
//   DELETE /api/sites/:id           delete a site (admin)
//   GET  /api/sites/:id/live        live readings from the site's Home Assistant
//   GET  /api/sites/:id/log         site log
//   POST /api/sites/:id/log         add a log entry (edit access)
//   POST /api/sites/:id/ask         ask Claude about the site (view access)
//   GET  /api/admin/users           team list (admin)
//   POST /api/admin/users           add employee, returns PIN once (admin)
//   GET  /api/admin/users/:id       employee profile + site access (admin)
//   PATCH /api/admin/users/:id      update employee (admin)
//   PUT  /api/admin/users/:id/access  set site ticks (admin)
//   POST /api/admin/users/:id/reset-pin (admin)
//   GET  /health                    no login

const express = require('express');
const fs = require('fs');
const path = require('path');
const db = require('./db');
const auth = require('./auth');
const ha = require('./ha');
const ai = require('./ai');
const { migrate } = require('./migrate');

const app = express();
app.use(express.json({ limit: '1mb' }));
app.use(auth.loadUser);

const PORT = process.env.PORT || 3000;

const UI_HTML = (() => {
  try {
    return fs.readFileSync(path.join(__dirname, 'ui.html'), 'utf8');
  } catch (err) {
    console.error('Failed to load ui.html:', err.message);
    return '<h1>Saturn Command Center</h1><p>UI file missing. Visit /health to check the server.</p>';
  }
})();

app.get('/', (req, res) => {
  res.set('Cache-Control', 'no-store');
  res.type('html').send(UI_HTML);
});

const SITE_TYPES = ['personal', 'saturn', 'customer'];
const SITE_STATUSES = ['ok', 'attention', 'fault', 'commissioning', 'offline'];
const SITE_TEXT_FIELDS = ['name', 'client', 'address', 'aroflo_job', 'summary', 'equipment', 'io_list', 'network', 'notes', 'ha_entities'];

function wrap(fn) {
  return (req, res) => fn(req, res).catch(err => {
    console.error('[ERROR]', req.method, req.path, err);
    res.status(err.status || 500).json({ error: err.expose ? err.message : 'Something went wrong' });
  });
}

function httpError(status, message) {
  const e = new Error(message);
  e.status = status;
  e.expose = true;
  return e;
}

// 'edit' | 'view' | null
async function siteAccess(user, siteId) {
  if (!user) return null;
  if (user.is_admin) return 'edit';
  const r = await db.query('SELECT can_edit FROM site_access WHERE user_id = $1 AND site_id = $2', [user.id, siteId]);
  if (!r.rows.length) return null;
  return r.rows[0].can_edit ? 'edit' : 'view';
}

async function loadSite(req, needed) {
  const id = parseInt(req.params.id, 10);
  if (!Number.isInteger(id)) throw httpError(404, 'Site not found');
  const access = await siteAccess(req.user, id);
  if (!access) throw httpError(404, 'Site not found'); // don't reveal sites they can't see
  if (needed === 'edit' && access !== 'edit') throw httpError(403, 'You have view-only access to this site');
  const r = await db.query('SELECT * FROM sites WHERE id = $1', [id]);
  if (!r.rows.length) throw httpError(404, 'Site not found');
  return { site: r.rows[0], access };
}

function publicSite(site, access) {
  const { ha_token_enc, ...rest } = site;
  return { ...rest, has_ha_token: !!ha_token_enc, access };
}

async function addLog(siteId, userId, entry) {
  await db.query('INSERT INTO site_log (site_id, user_id, entry) VALUES ($1, $2, $3)', [siteId, userId, entry]);
}

// --- Auth -------------------------------------------------------------------

app.post('/api/login', wrap(async (req, res) => {
  const ip = (req.headers['x-forwarded-for'] || req.socket.remoteAddress || '').split(',')[0].trim();
  if (auth.rateLimited(ip)) return res.status(429).json({ error: 'Too many attempts. Try again in an hour.' });
  const pin = String((req.body && req.body.pin) || '').trim();
  if (!/^\d{6}$/.test(pin)) return res.status(400).json({ error: 'PIN must be 6 digits' });
  const result = await db.query('SELECT id, name, pin_hash, is_admin FROM users WHERE active = TRUE');
  let matched = null;
  for (const u of result.rows) {
    if (await auth.verifyPin(pin, u.pin_hash)) { matched = u; break; }
  }
  if (!matched) return res.status(401).json({ error: 'Invalid PIN' });
  auth.clearRateLimit(ip);
  await db.query('UPDATE users SET last_login_at = NOW() WHERE id = $1', [matched.id]);
  const token = auth.signToken({ uid: matched.id, exp: Date.now() + auth.SESSION_TTL_MS });
  auth.setCookie(res, auth.COOKIE_NAME, token, { maxAge: auth.SESSION_TTL_MS });
  res.json({ ok: true, user: { id: matched.id, name: matched.name, is_admin: matched.is_admin } });
}));

app.post('/api/logout', (req, res) => {
  auth.clearCookie(res, auth.COOKIE_NAME);
  res.json({ ok: true });
});

app.get('/api/me', (req, res) => {
  if (!req.user) return res.status(401).json({ error: 'Not authenticated' });
  const u = req.user;
  res.json({
    id: u.id, name: u.name, email: u.email, phone: u.phone, role_title: u.role_title,
    is_admin: u.is_admin, can_add_sites: u.is_admin || u.can_add_sites,
    ai_enabled: !!process.env.ANTHROPIC_API_KEY,
  });
});

// --- Sites ------------------------------------------------------------------

app.get('/api/sites', auth.requireAuth, wrap(async (req, res) => {
  const cols = `s.id, s.name, s.site_type, s.client, s.address, s.aroflo_job, s.status, s.summary, s.updated_at,
                (s.ha_url IS NOT NULL AND s.ha_token_enc IS NOT NULL) AS has_live`;
  let r;
  if (req.user.is_admin) {
    r = await db.query(`SELECT ${cols}, 'edit' AS access FROM sites s ORDER BY s.name`);
  } else {
    r = await db.query(
      `SELECT ${cols}, CASE WHEN a.can_edit THEN 'edit' ELSE 'view' END AS access
         FROM sites s JOIN site_access a ON a.site_id = s.id AND a.user_id = $1
        ORDER BY s.name`, [req.user.id]);
  }
  res.json(r.rows);
}));

function readSiteBody(body, { creating }) {
  const out = {};
  for (const f of SITE_TEXT_FIELDS) {
    if (body[f] !== undefined) out[f] = String(body[f] || '').trim() || null;
  }
  if (body.site_type !== undefined) {
    if (!SITE_TYPES.includes(body.site_type)) throw httpError(400, 'Unknown site type');
    out.site_type = body.site_type;
  }
  if (body.status !== undefined) {
    if (!SITE_STATUSES.includes(body.status)) throw httpError(400, 'Unknown status');
    out.status = body.status;
  }
  if (body.ha_url !== undefined) {
    try { out.ha_url = ha.cleanUrl(body.ha_url); } catch (e) { throw httpError(400, e.message); }
  }
  if (body.ha_token !== undefined && String(body.ha_token).trim()) {
    try { out.ha_token_enc = ha.encrypt(String(body.ha_token).trim()); } catch (e) { throw httpError(400, e.message); }
  }
  if (body.clear_ha_token === true) out.ha_token_enc = null;
  if (creating && !out.name) throw httpError(400, 'Site name is required');
  if (out.name === null) throw httpError(400, 'Site name is required');
  return out;
}

app.post('/api/sites', auth.requireAuth, wrap(async (req, res) => {
  if (!req.user.is_admin && !req.user.can_add_sites) throw httpError(403, "You don't have permission to add sites");
  const data = readSiteBody(req.body || {}, { creating: true });
  data.created_by = req.user.id;
  const keys = Object.keys(data);
  const r = await db.query(
    `INSERT INTO sites (${keys.join(', ')}) VALUES (${keys.map((_, i) => '$' + (i + 1)).join(', ')}) RETURNING *`,
    keys.map(k => data[k]));
  const site = r.rows[0];
  if (!req.user.is_admin) {
    await db.query('INSERT INTO site_access (user_id, site_id, can_edit) VALUES ($1, $2, TRUE) ON CONFLICT DO NOTHING', [req.user.id, site.id]);
  }
  await addLog(site.id, req.user.id, 'Site created');
  res.json(publicSite(site, 'edit'));
}));

app.get('/api/sites/:id', auth.requireAuth, wrap(async (req, res) => {
  const { site, access } = await loadSite(req, 'view');
  res.json(publicSite(site, access));
}));

app.patch('/api/sites/:id', auth.requireAuth, wrap(async (req, res) => {
  const { site } = await loadSite(req, 'edit');
  const data = readSiteBody(req.body || {}, { creating: false });
  const keys = Object.keys(data);
  if (!keys.length) return res.json(publicSite(site, 'edit'));
  const sets = keys.map((k, i) => `${k} = $${i + 1}`).concat('updated_at = NOW()');
  const r = await db.query(
    `UPDATE sites SET ${sets.join(', ')} WHERE id = $${keys.length + 1} RETURNING *`,
    keys.map(k => data[k]).concat(site.id));
  const changed = keys.map(k => (k === 'ha_token_enc' ? 'Home Assistant token' : k.replace(/_/g, ' ')));
  if (data.status && data.status !== site.status) {
    await addLog(site.id, req.user.id, `Status changed from ${site.status} to ${data.status}`);
  } else {
    await addLog(site.id, req.user.id, 'Updated: ' + changed.join(', '));
  }
  res.json(publicSite(r.rows[0], 'edit'));
}));

app.delete('/api/sites/:id', auth.requireAdmin, wrap(async (req, res) => {
  const r = await db.query('DELETE FROM sites WHERE id = $1 RETURNING id', [req.params.id]);
  if (!r.rowCount) throw httpError(404, 'Site not found');
  res.json({ ok: true });
}));

app.get('/api/sites/:id/live', auth.requireAuth, wrap(async (req, res) => {
  const { site } = await loadSite(req, 'view');
  res.json(await ha.fetchLive(site));
}));

async function recentLog(siteId, limit) {
  const r = await db.query(
    `SELECT l.id, l.entry, l.created_at, u.name AS user_name
       FROM site_log l LEFT JOIN users u ON u.id = l.user_id
      WHERE l.site_id = $1 ORDER BY l.created_at DESC, l.id DESC LIMIT $2`, [siteId, limit]);
  return r.rows;
}

app.get('/api/sites/:id/log', auth.requireAuth, wrap(async (req, res) => {
  const { site } = await loadSite(req, 'view');
  res.json(await recentLog(site.id, 200));
}));

app.post('/api/sites/:id/log', auth.requireAuth, wrap(async (req, res) => {
  const { site } = await loadSite(req, 'edit');
  const entry = String((req.body && req.body.entry) || '').trim();
  if (!entry) throw httpError(400, 'Write something first');
  await addLog(site.id, req.user.id, entry.slice(0, 4000));
  res.json({ ok: true });
}));

app.post('/api/sites/:id/ask', auth.requireAuth, wrap(async (req, res) => {
  const { site } = await loadSite(req, 'view');
  const question = String((req.body && req.body.question) || '').trim();
  if (!question) throw httpError(400, 'Ask a question first');
  const history = Array.isArray(req.body.history) ? req.body.history : [];
  const [log, live] = await Promise.all([recentLog(site.id, 30), ha.fetchLive(site)]);
  const result = await ai.askAboutSite({ site, log, live, question, history });
  res.status(result.ok ? 200 : 503).json(result);
}));

// --- Team (admin) -----------------------------------------------------------

app.get('/api/admin/users', auth.requireAdmin, wrap(async (req, res) => {
  const r = await db.query(
    `SELECT u.id, u.name, u.email, u.phone, u.role_title, u.is_admin, u.can_add_sites, u.active,
            u.created_at, u.last_login_at,
            COUNT(a.site_id)::int AS site_count,
            COUNT(a.site_id) FILTER (WHERE a.can_edit)::int AS edit_count
       FROM users u LEFT JOIN site_access a ON a.user_id = u.id
      GROUP BY u.id ORDER BY u.active DESC, u.name`);
  res.json(r.rows);
}));

app.post('/api/admin/users', auth.requireAdmin, wrap(async (req, res) => {
  const b = req.body || {};
  const name = String(b.name || '').trim();
  if (!name) throw httpError(400, 'Name is required');
  const pin = auth.generatePin();
  const pinHash = await auth.hashPin(pin);
  try {
    const r = await db.query(
      `INSERT INTO users (name, email, phone, role_title, pin_hash, is_admin, can_add_sites, active)
       VALUES ($1, $2, $3, $4, $5, $6, $7, TRUE)
       RETURNING id, name`,
      [name, String(b.email || '').trim() || null, String(b.phone || '').trim() || null,
       String(b.role_title || '').trim() || null, pinHash, !!b.is_admin, !!b.can_add_sites]);
    res.json({ ok: true, user: r.rows[0], pin }); // PIN shown once only
  } catch (err) {
    if (err.code === '23505') throw httpError(409, 'Someone with that name already exists');
    throw err;
  }
}));

app.get('/api/admin/users/:id', auth.requireAdmin, wrap(async (req, res) => {
  const u = await db.query(
    `SELECT id, name, email, phone, role_title, is_admin, can_add_sites, active, created_at, last_login_at
       FROM users WHERE id = $1`, [req.params.id]);
  if (!u.rows.length) throw httpError(404, 'Not found');
  const sites = await db.query(
    `SELECT s.id, s.name, s.site_type, s.client,
            CASE WHEN a.site_id IS NULL THEN 'none' WHEN a.can_edit THEN 'edit' ELSE 'view' END AS access
       FROM sites s LEFT JOIN site_access a ON a.site_id = s.id AND a.user_id = $1
      ORDER BY s.name`, [req.params.id]);
  res.json({ user: u.rows[0], sites: sites.rows });
}));

app.patch('/api/admin/users/:id', auth.requireAdmin, wrap(async (req, res) => {
  const b = req.body || {};
  const id = parseInt(req.params.id, 10);
  if (id === req.user.id && (b.active === false || b.is_admin === false)) {
    throw httpError(400, "You can't remove your own admin rights or deactivate yourself");
  }
  const fields = [];
  const values = [];
  for (const k of ['active', 'is_admin', 'can_add_sites']) {
    if (typeof b[k] === 'boolean') { values.push(b[k]); fields.push(`${k} = $${values.length}`); }
  }
  for (const k of ['name', 'email', 'phone', 'role_title']) {
    if (b[k] !== undefined) {
      const v = String(b[k] || '').trim() || null;
      if (k === 'name' && !v) throw httpError(400, 'Name is required');
      values.push(v); fields.push(`${k} = $${values.length}`);
    }
  }
  if (!fields.length) throw httpError(400, 'Nothing to update');
  values.push(id);
  try {
    const r = await db.query(`UPDATE users SET ${fields.join(', ')} WHERE id = $${values.length} RETURNING id`, values);
    if (!r.rowCount) throw httpError(404, 'Not found');
  } catch (err) {
    if (err.code === '23505') throw httpError(409, 'Someone with that name already exists');
    throw err;
  }
  res.json({ ok: true });
}));

// body: { access: [{ site_id, level: 'none' | 'view' | 'edit' }] }
app.put('/api/admin/users/:id/access', auth.requireAdmin, wrap(async (req, res) => {
  const userId = parseInt(req.params.id, 10);
  const list = Array.isArray(req.body && req.body.access) ? req.body.access : null;
  if (!list) throw httpError(400, 'access list required');
  const client = await db.pool.connect();
  try {
    await client.query('BEGIN');
    for (const item of list) {
      const siteId = parseInt(item.site_id, 10);
      if (!Number.isInteger(siteId)) continue;
      if (item.level === 'view' || item.level === 'edit') {
        await client.query(
          `INSERT INTO site_access (user_id, site_id, can_edit) VALUES ($1, $2, $3)
           ON CONFLICT (user_id, site_id) DO UPDATE SET can_edit = EXCLUDED.can_edit`,
          [userId, siteId, item.level === 'edit']);
      } else {
        await client.query('DELETE FROM site_access WHERE user_id = $1 AND site_id = $2', [userId, siteId]);
      }
    }
    await client.query('COMMIT');
  } catch (err) {
    await client.query('ROLLBACK');
    throw err;
  } finally {
    client.release();
  }
  res.json({ ok: true });
}));

app.post('/api/admin/users/:id/reset-pin', auth.requireAdmin, wrap(async (req, res) => {
  const pin = auth.generatePin();
  const r = await db.query('UPDATE users SET pin_hash = $1 WHERE id = $2 RETURNING id, name', [await auth.hashPin(pin), req.params.id]);
  if (!r.rowCount) throw httpError(404, 'Not found');
  res.json({ ok: true, user: r.rows[0], pin });
}));

// --- Health -------------------------------------------------------------------

app.get('/health', async (req, res) => {
  try {
    const r = await db.query('SELECT NOW() AS server_time');
    res.json({ status: 'ok', app: 'saturn-command-center', database: 'connected', server_time: r.rows[0].server_time });
  } catch (err) {
    res.status(500).json({ status: 'error', database: 'disconnected', message: err.message });
  }
});

// --- Start --------------------------------------------------------------------

migrate()
  .then(() => auth.bootstrapAdmin())
  .then(() => {
    app.listen(PORT, () => console.log(`Saturn Command Center listening on port ${PORT}`));
  })
  .catch(err => {
    console.error('[STARTUP] failed:', err);
    process.exit(1);
  });
