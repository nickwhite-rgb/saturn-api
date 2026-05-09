// index.js — Saturn OS API
// Endpoints:
//   GET  /health              — server + DB liveness check
//   GET  /test-aroflo         — debug: ping AroFlo and show raw response
//   POST /sync/run            — manually trigger a purchase-order sync (also accepts GET for easy browser test)
//   GET  /sync/status         — last 10 sync attempts
//   GET  /purchases           — search/sort/filter purchase line items
//   GET  /purchases/stats     — summary tile numbers (total POs, lines, spend, suppliers)
//   GET  /purchases/filters   — distinct supplier + category lists for dropdowns

const express = require('express');
const cron = require('node-cron');
const fs = require('fs');
const path = require('path');
const db = require('./db');
const sync = require('./sync');
const auth = require('./auth');
const { importCsv } = require('./import');

const app = express();
app.use(express.json());

// All requests get req.user populated (or null if not logged in)
app.use(auth.loadUser);

// Bootstrap the first admin from ADMIN_INITIAL_PIN env var if no users exist
auth.bootstrapAdmin().catch(err => console.error('[AUTH] bootstrap failed:', err));

const PORT = process.env.PORT || 3000;

// Load the search UI HTML at startup
const UI_HTML = (() => {
  try {
    return fs.readFileSync(path.join(__dirname, 'ui.html'), 'utf8');
  } catch (err) {
    console.error('Failed to load ui.html:', err.message);
    return '<h1>Saturn OS</h1><p>UI file missing — visit /health to verify the API is running.</p>';
  }
})();

// Search UI at root (UI itself handles login state via /api/me)
app.get('/', (req, res) => {
  res.type('html').send(UI_HTML);
});

// --- Auth endpoints ---------------------------------------------------------

app.post('/api/login', async (req, res) => {
  const ip = (req.headers['x-forwarded-for'] || req.socket.remoteAddress || '').split(',')[0].trim();
  if (auth.rateLimited(ip)) {
    return res.status(429).json({ error: 'Too many attempts. Try again in an hour.' });
  }
  const pin = String((req.body && req.body.pin) || '').trim();
  if (!/^\d{6}$/.test(pin)) {
    return res.status(400).json({ error: 'PIN must be 6 digits' });
  }
  try {
    const result = await db.query(
      `SELECT id, name, pin_hash, is_admin FROM users WHERE active = TRUE`
    );
    let matchedUser = null;
    for (const u of result.rows) {
      if (await auth.verifyPin(pin, u.pin_hash)) { matchedUser = u; break; }
    }
    if (!matchedUser) {
      return res.status(401).json({ error: 'Invalid PIN' });
    }
    auth.clearRateLimit(ip);
    await db.query('UPDATE users SET last_login_at = NOW() WHERE id = $1', [matchedUser.id]);
    const token = auth.signToken({
      uid: matchedUser.id,
      exp: Date.now() + auth.SESSION_TTL_MS,
    });
    auth.setCookie(res, auth.COOKIE_NAME, token, { maxAge: auth.SESSION_TTL_MS });
    res.json({
      ok: true,
      user: { id: matchedUser.id, name: matchedUser.name, is_admin: matchedUser.is_admin },
    });
  } catch (err) {
    console.error('[LOGIN]', err);
    res.status(500).json({ error: 'Login failed' });
  }
});

app.post('/api/logout', (req, res) => {
  auth.clearCookie(res, auth.COOKIE_NAME);
  res.json({ ok: true });
});

app.get('/api/me', (req, res) => {
  if (!req.user) return res.status(401).json({ error: 'Not authenticated' });
  res.json({
    id: req.user.id,
    name: req.user.name,
    email: req.user.email,
    phone: req.user.phone,
    is_admin: req.user.is_admin,
  });
});

// --- Admin endpoints (require is_admin) -------------------------------------

app.get('/api/admin/users', auth.requireAdmin, async (req, res) => {
  try {
    const result = await db.query(
      `SELECT id, name, email, phone, is_admin, active, created_at, last_login_at
         FROM users
        ORDER BY active DESC, name`
    );
    res.json(result.rows);
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

app.post('/api/admin/users', auth.requireAdmin, async (req, res) => {
  const name = String((req.body && req.body.name) || '').trim();
  const email = String((req.body && req.body.email) || '').trim() || null;
  const phone = String((req.body && req.body.phone) || '').trim() || null;
  const isAdmin = !!(req.body && req.body.is_admin);
  if (!name) return res.status(400).json({ error: 'name is required' });
  try {
    const pin = auth.generatePin();
    const pinHash = await auth.hashPin(pin);
    const result = await db.query(
      `INSERT INTO users (name, email, phone, pin_hash, is_admin, active)
       VALUES ($1, $2, $3, $4, $5, TRUE)
       RETURNING id, name, email, phone, is_admin, active, created_at`,
      [name, email, phone, pinHash, isAdmin]
    );
    // Return the PIN ONCE — it's never retrievable again
    res.json({ ok: true, user: result.rows[0], pin });
  } catch (err) {
    if (err.code === '23505') {
      return res.status(409).json({ error: 'A user with that name already exists' });
    }
    console.error('[ADMIN/CREATE]', err);
    res.status(500).json({ error: err.message });
  }
});

app.post('/api/admin/users/:id/reset-pin', auth.requireAdmin, async (req, res) => {
  try {
    const pin = auth.generatePin();
    const pinHash = await auth.hashPin(pin);
    const result = await db.query(
      `UPDATE users SET pin_hash = $1 WHERE id = $2 RETURNING id, name`,
      [pinHash, req.params.id]
    );
    if (result.rowCount === 0) return res.status(404).json({ error: 'Not found' });
    res.json({ ok: true, user: result.rows[0], pin });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

app.patch('/api/admin/users/:id', auth.requireAdmin, async (req, res) => {
  const { active, is_admin } = req.body || {};
  const fields = [];
  const values = [];
  let i = 1;
  if (typeof active === 'boolean')   { fields.push(`active = $${i}`);   values.push(active);   i++; }
  if (typeof is_admin === 'boolean') { fields.push(`is_admin = $${i}`); values.push(is_admin); i++; }
  if (fields.length === 0) return res.status(400).json({ error: 'Nothing to update' });
  values.push(req.params.id);
  try {
    const result = await db.query(
      `UPDATE users SET ${fields.join(', ')} WHERE id = $${i} RETURNING id, name, is_admin, active`,
      values
    );
    if (result.rowCount === 0) return res.status(404).json({ error: 'Not found' });
    // Don't let the admin deactivate themselves
    if (req.user.id === result.rows[0].id && result.rows[0].active === false) {
      // revert
      await db.query(`UPDATE users SET active = TRUE WHERE id = $1`, [result.rows[0].id]);
      return res.status(400).json({ error: "You can't deactivate your own account" });
    }
    res.json({ ok: true, user: result.rows[0] });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

// --- Health -----------------------------------------------------------------
app.get('/health', async (req, res) => {
  try {
    const result = await db.query('SELECT NOW() AS server_time');
    res.json({ status: 'ok', database: 'connected', server_time: result.rows[0].server_time });
  } catch (err) {
    res.status(500).json({ status: 'error', database: 'disconnected', message: err.message });
  }
});

// --- AroFlo debug ping ------------------------------------------------------
// Useful for sanity-checking endpoints, joins, etc. Not used in production paths.
app.get('/test-aroflo', auth.requireAdmin, async (req, res) => {
  const u = process.env.AROFLO_U_ENCODED;
  const p = process.env.AROFLO_P_ENCODED;
  const org = process.env.AROFLO_ORG_ENCODED;
  if (!u || !p || !org) {
    return res.status(500).json({ error: 'AroFlo credentials not set in env' });
  }

  const zone = req.query.zone || 'purchaseorders';
  const page = req.query.page || '1';
  const join = req.query.join || 'lineitems';

  const params = new URLSearchParams();
  params.append('zone', zone);
  params.append('page', page);
  if (join) params.append('join', join);
  params.append('uencoded', u);
  params.append('pencoded', p);
  params.append('orgencoded', org);

  const url = `https://api.aroflo.com/?${params.toString()}`;
  try {
    const response = await fetch(url, { method: 'GET' });
    const text = await response.text();
    res.json({
      status: response.status,
      contentType: response.headers.get('content-type') || '',
      url: `https://api.aroflo.com/?zone=${zone}&page=${page}&join=${join}&uencoded=***&pencoded=***&orgencoded=***`,
      bodyPreview: text.slice(0, 4000),
    });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

// --- Sync trigger -----------------------------------------------------------
async function handleSyncRun(req, res) {
  try {
    const result = await sync.syncPurchaseOrders({ trigger: 'manual' });
    res.json(result);
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
}
// Accept both POST (proper) and GET (easy browser test)
app.post('/sync/run', auth.requireAdmin, handleSyncRun);
app.get('/sync/run', auth.requireAdmin, handleSyncRun);

app.get('/sync/status', auth.requireAuth, async (req, res) => {
  try {
    const result = await db.query(
      `SELECT id, started_at, finished_at, status, trigger,
              purchase_orders, lines_inserted, lines_updated, duration_ms, error
         FROM sync_runs
        ORDER BY id DESC
        LIMIT 10`
    );
    res.json(result.rows);
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

// --- Purchases: search / sort / filter / paginate ---------------------------
app.get('/purchases', auth.requireAuth, async (req, res) => {
  const {
    search = '',
    supplier = '',
    category = '',
    from = '',
    to = '',
    sort = 'date_invoiced',
    direction = 'desc',
    page = '1',
    limit = '50',
  } = req.query;

  const allowedSorts = [
    'date_invoiced', 'po_number', 'supplier_invoice_no', 'supplier',
    'part_no', 'description', 'category', 'qty', 'unit_price', 'line_total',
  ];
  const sortCol = allowedSorts.includes(sort) ? sort : 'date_invoiced';
  const dir = String(direction).toLowerCase() === 'asc' ? 'ASC' : 'DESC';

  const conditions = [];
  const params = [];
  let i = 1;

  if (search) {
    conditions.push(
      `(description ILIKE $${i} OR part_no ILIKE $${i} OR po_number ILIKE $${i} OR supplier_invoice_no ILIKE $${i})`
    );
    params.push(`%${search}%`);
    i++;
  }
  if (supplier) { conditions.push(`supplier = $${i}`);  params.push(supplier);  i++; }
  if (category) { conditions.push(`category = $${i}`);  params.push(category);  i++; }
  if (from)     { conditions.push(`date_invoiced >= $${i}`); params.push(from); i++; }
  if (to)       { conditions.push(`date_invoiced <= $${i}`); params.push(to);   i++; }

  const where = conditions.length ? 'WHERE ' + conditions.join(' AND ') : '';
  const lim = Math.min(500, Math.max(1, parseInt(limit, 10) || 50));
  const pg = Math.max(1, parseInt(page, 10) || 1);
  const offset = (pg - 1) * lim;

  try {
    const dataQuery = `
      SELECT line_id, date_invoiced, po_number, supplier_invoice_no, supplier,
             part_no, description, category, qty, unit_price, line_total
        FROM purchase_lines
        ${where}
        ORDER BY ${sortCol} ${dir} NULLS LAST
        LIMIT ${lim} OFFSET ${offset}
    `;
    const countQuery = `SELECT COUNT(*) AS n FROM purchase_lines ${where}`;
    const [data, count] = await Promise.all([
      db.query(dataQuery, params),
      db.query(countQuery, params),
    ]);
    res.json({
      total: parseInt(count.rows[0].n, 10),
      page: pg,
      limit: lim,
      sort: sortCol,
      direction: dir,
      results: data.rows,
    });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

// --- Purchases: summary tiles -----------------------------------------------
app.get('/purchases/stats', auth.requireAuth, async (req, res) => {
  try {
    const [pos, lines, spend, suppliers, lastSync] = await Promise.all([
      db.query(`SELECT COUNT(DISTINCT po_number) AS n FROM purchase_lines WHERE po_number IS NOT NULL`),
      db.query(`SELECT COUNT(*) AS n FROM purchase_lines`),
      db.query(`SELECT COALESCE(SUM(line_total), 0) AS n FROM purchase_lines`),
      db.query(`SELECT COUNT(DISTINCT supplier) AS n FROM purchase_lines WHERE supplier IS NOT NULL`),
      db.query(`SELECT finished_at FROM sync_runs WHERE status='ok' ORDER BY id DESC LIMIT 1`),
    ]);
    res.json({
      purchase_orders: parseInt(pos.rows[0].n, 10),
      line_items: parseInt(lines.rows[0].n, 10),
      total_spend: parseFloat(spend.rows[0].n),
      suppliers: parseInt(suppliers.rows[0].n, 10),
      last_synced_at: lastSync.rows[0] ? lastSync.rows[0].finished_at : null,
    });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

// --- Filter dropdown options ------------------------------------------------
app.get('/purchases/filters', auth.requireAuth, async (req, res) => {
  try {
    const [suppliers, categories] = await Promise.all([
      db.query(`SELECT DISTINCT supplier FROM purchase_lines WHERE supplier IS NOT NULL ORDER BY supplier`),
      db.query(`SELECT DISTINCT category FROM purchase_lines WHERE category IS NOT NULL ORDER BY category`),
    ]);
    res.json({
      suppliers: suppliers.rows.map(r => r.supplier),
      categories: categories.rows.map(r => r.category),
    });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

// --- CSV bulk import --------------------------------------------------------
// Drag-and-drop browser page at /import — POSTs the CSV body to /import/csv.

app.get('/import', auth.requireAdmin, (req, res) => {
  res.type('html').send(`<!doctype html>
<html><head><meta charset="utf-8"><title>Saturn OS — CSV Import</title>
<style>
  body { font-family: -apple-system, system-ui, Segoe UI, sans-serif; max-width: 720px; margin: 40px auto; padding: 0 20px; color: #111; }
  h1 { font-size: 22px; font-weight: 500; margin: 0 0 8px; }
  p.muted { color: #555; margin: 0 0 24px; font-size: 14px; }
  .drop { border: 1.5px dashed #bbb; border-radius: 12px; padding: 40px 20px; text-align: center; background: #fafafa; }
  .drop.over { background: #eef5ff; border-color: #378add; }
  input[type=file] { margin: 16px 0; }
  button { padding: 10px 20px; background: #111; color: #fff; border: 0; border-radius: 6px; font-size: 14px; cursor: pointer; }
  button:disabled { opacity: 0.5; cursor: not-allowed; }
  pre { background: #f4f4f4; padding: 16px; border-radius: 6px; font-size: 12px; overflow-x: auto; white-space: pre-wrap; word-break: break-word; }
  .warn { color: #985e00; }
  .ok { color: #0c8a3a; }
  .err { color: #c1272d; }
</style></head><body>
<h1>Import purchase orders</h1>
<p class="muted">Drop your AroFlo CSV export here. Same file can be imported again — duplicates upsert cleanly.</p>
<div class="drop" id="drop">
  <div>Drop a .csv file here, or</div>
  <input type="file" id="csv" accept=".csv,text/csv" />
  <div><button id="upload" disabled>Upload</button></div>
  <div id="filename" class="muted" style="margin-top: 12px;"></div>
</div>
<pre id="result" style="display:none"></pre>
<script>
  const drop = document.getElementById('drop');
  const fileIn = document.getElementById('csv');
  const btn = document.getElementById('upload');
  const fname = document.getElementById('filename');
  const out = document.getElementById('result');
  let file = null;
  function setFile(f) { file = f; fname.textContent = f ? f.name + ' (' + (f.size/1024).toFixed(1) + ' KB)' : ''; btn.disabled = !f; }
  fileIn.addEventListener('change', () => setFile(fileIn.files[0]));
  ['dragenter','dragover'].forEach(e => drop.addEventListener(e, ev => { ev.preventDefault(); drop.classList.add('over'); }));
  ['dragleave','drop'].forEach(e => drop.addEventListener(e, ev => { ev.preventDefault(); drop.classList.remove('over'); }));
  drop.addEventListener('drop', ev => { ev.preventDefault(); if (ev.dataTransfer.files[0]) setFile(ev.dataTransfer.files[0]); });
  btn.addEventListener('click', async () => {
    if (!file) return;
    btn.disabled = true; out.style.display = 'block'; out.textContent = 'Reading file...';
    const text = await file.text();
    out.textContent = 'Uploading and importing... (this can take a few minutes for large files)';
    try {
      const res = await fetch('/import/csv', { method: 'POST', headers: { 'Content-Type': 'text/csv' }, body: text });
      const json = await res.json();
      out.textContent = JSON.stringify(json, null, 2);
      if (json.ok) out.classList.add('ok');
      else out.classList.add('err');
    } catch (err) {
      out.textContent = 'Error: ' + err.message;
      out.classList.add('err');
    }
    btn.disabled = false;
  });
</script>
</body></html>`);
});

app.post('/import/csv', auth.requireAdmin, express.text({ type: '*/*', limit: '200mb' }), async (req, res) => {
  const csv = req.body;
  if (!csv || typeof csv !== 'string' || csv.length === 0) {
    return res.status(400).json({ error: 'No CSV body received. POST text/csv content.' });
  }
  try {
    const result = await importCsv(csv);
    res.json(result);
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

// --- Cron: 2am Brisbane, weekdays only (Mon-Fri) ----------------------------
// Cron format:  minute  hour  day-of-month  month  day-of-week
//               0       2     *             *      1-5         = 2:00 AM Mon-Fri
if (process.env.ENABLE_CRON !== 'false') {
  cron.schedule('0 2 * * 1-5', () => {
    console.log('[CRON] 2:00 AM Brisbane weekday sync starting');
    sync.syncPurchaseOrders({ trigger: 'cron' })
      .then(r => console.log('[CRON] sync done', r))
      .catch(err => console.error('[CRON] sync failed:', err));
  }, { timezone: 'Australia/Brisbane' });

  console.log('[CRON] scheduled for 02:00 Mon-Fri Australia/Brisbane');
}

app.listen(PORT, () => {
  console.log(`Saturn OS API listening on port ${PORT}`);
});
