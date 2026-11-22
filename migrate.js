// migrate.js — Saturn Command Center database setup
//
// Runs automatically every time the app starts. Safe to re-run:
//   - creates the Saturn Command Center tables if they don't exist
//   - removes the old Saturn OS purchase-history tables (one-off, logged)
//   - keeps the users table so existing PINs keep working

const db = require('./db');

async function migrate() {
  // --- users (existing Saturn OS table, extended) ---------------------------
  await db.query(`
    CREATE TABLE IF NOT EXISTS users (
      id             SERIAL PRIMARY KEY,
      name           TEXT NOT NULL UNIQUE,
      email          TEXT,
      phone          TEXT,
      pin_hash       TEXT NOT NULL,
      is_admin       BOOLEAN NOT NULL DEFAULT FALSE,
      active         BOOLEAN NOT NULL DEFAULT TRUE,
      created_at     TIMESTAMPTZ NOT NULL DEFAULT NOW(),
      last_login_at  TIMESTAMPTZ
    )`);
  await db.query(`ALTER TABLE users ADD COLUMN IF NOT EXISTS can_add_sites BOOLEAN NOT NULL DEFAULT FALSE`);
  await db.query(`ALTER TABLE users ADD COLUMN IF NOT EXISTS role_title TEXT`);
  await db.query(`ALTER TABLE users ADD COLUMN IF NOT EXISTS can_force BOOLEAN NOT NULL DEFAULT FALSE`);

  // --- remove old Saturn OS purchase-history data ---------------------------
  const old = await db.query(
    `SELECT table_name FROM information_schema.tables
      WHERE table_schema = 'public' AND table_name IN ('purchase_lines', 'sync_runs', 'items')`
  );
  if (old.rows.length) {
    await db.query('DROP TABLE IF EXISTS purchase_lines, sync_runs, items CASCADE');
    console.log('[MIGRATE] Removed old Saturn OS tables:', old.rows.map(r => r.table_name).join(', '));
  }

  // --- sites ------------------------------------------------------------------
  await db.query(`
    CREATE TABLE IF NOT EXISTS sites (
      id           SERIAL PRIMARY KEY,
      name         TEXT NOT NULL,
      site_type    TEXT NOT NULL DEFAULT 'customer',   -- personal | saturn | customer
      client       TEXT,
      address      TEXT,
      aroflo_job   TEXT,
      status       TEXT NOT NULL DEFAULT 'ok',          -- ok | attention | fault | commissioning | offline
      summary      TEXT,
      equipment    TEXT,
      io_list      TEXT,
      network      TEXT,
      notes        TEXT,
      ha_url       TEXT,
      ha_token_enc TEXT,                                -- encrypted, never sent to the browser
      ha_entities  TEXT,                                -- one Home Assistant entity id per line
      created_by   INT REFERENCES users(id) ON DELETE SET NULL,
      created_at   TIMESTAMPTZ NOT NULL DEFAULT NOW(),
      updated_at   TIMESTAMPTZ NOT NULL DEFAULT NOW()
    )`);

  // I/O page + forcing (Saturn PLC standard v1, read/written through the site's Home Assistant)
  await db.query(`ALTER TABLE sites ADD COLUMN IF NOT EXISTS io_points JSONB NOT NULL DEFAULT '[]'::jsonb`);
  await db.query(`ALTER TABLE sites ADD COLUMN IF NOT EXISTS io_prefix TEXT`);   // HA entity prefix, e.g. 'shed'
  await db.query(`ALTER TABLE sites ADD COLUMN IF NOT EXISTS io_hub TEXT`);      // HA modbus hub name, e.g. 'shed_plc'
  await db.query(`ALTER TABLE sites ADD COLUMN IF NOT EXISTS io_slave INT NOT NULL DEFAULT 1`);
  await db.query(`ALTER TABLE sites ADD COLUMN IF NOT EXISTS force_changed_at TIMESTAMPTZ`);

  // --- who can see / edit which site -----------------------------------------
  await db.query(`
    CREATE TABLE IF NOT EXISTS site_access (
      user_id  INT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
      site_id  INT NOT NULL REFERENCES sites(id) ON DELETE CASCADE,
      can_edit BOOLEAN NOT NULL DEFAULT FALSE,
      PRIMARY KEY (user_id, site_id)
    )`);

  // --- site log (changes, visits, faults, notes) -----------------------------
  await db.query(`
    CREATE TABLE IF NOT EXISTS site_log (
      id          SERIAL PRIMARY KEY,
      site_id     INT NOT NULL REFERENCES sites(id) ON DELETE CASCADE,
      user_id     INT REFERENCES users(id) ON DELETE SET NULL,
      entry       TEXT NOT NULL,
      created_at  TIMESTAMPTZ NOT NULL DEFAULT NOW()
    )`);
  await db.query(`CREATE INDEX IF NOT EXISTS idx_site_log_site ON site_log (site_id, created_at DESC)`);

  // --- active I/O forces (one row per forced point) -------------------------
  await db.query(`
    CREATE TABLE IF NOT EXISTS site_forces (
      site_id    INT NOT NULL REFERENCES sites(id) ON DELETE CASCADE,
      kind       TEXT NOT NULL,            -- DI | DQ | AI
      channel    INT NOT NULL,
      value      INT NOT NULL,             -- 0/1 for DI/DQ, raw counts for AI
      eng_value  TEXT,                     -- what the person typed, e.g. '5.0 bar'
      forced_by  INT REFERENCES users(id) ON DELETE SET NULL,
      forced_at  TIMESTAMPTZ NOT NULL DEFAULT NOW(),
      PRIMARY KEY (site_id, kind, channel)
    )`);

  console.log('[MIGRATE] Database ready');
}

module.exports = { migrate };
