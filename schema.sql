-- schema.sql
-- Saturn OS Module 1: Purchase History
--
-- Run this in Railway's Postgres -> Database -> Query window.
-- Safe to re-run: drops the test items table, creates everything fresh.

-- Clean up test data from earlier
DROP TABLE IF EXISTS items;

-- Trigram extension for fast fuzzy search on description
CREATE EXTENSION IF NOT EXISTS pg_trgm;

-- ----------------------------------------------------------------------------
-- purchase_lines: one row per line item from every AroFlo purchase order
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS purchase_lines (
  -- AroFlo's stable line item identifier (used for upsert)
  line_id              TEXT PRIMARY KEY,

  -- Columns that map directly to Nick's spec
  date_invoiced        TIMESTAMPTZ,
  po_number            TEXT,
  supplier_invoice_no  TEXT,
  supplier             TEXT,
  part_no              TEXT,
  description          TEXT,
  category             TEXT,
  qty                  NUMERIC(14,4),
  unit_price           NUMERIC(14,4),
  line_total           NUMERIC(14,4),

  -- Useful extras for joining/filtering later (not shown in UI by default)
  purchase_order_id    TEXT,
  supplier_org_id      TEXT,
  task_id              TEXT,
  status               TEXT,

  -- Where this row came from: 'api' (cron sync) or 'csv' (bulk import)
  source               TEXT DEFAULT 'api',

  -- Sync metadata
  synced_at            TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

-- Indexes for fast filtering / sorting
CREATE INDEX IF NOT EXISTS idx_purchase_lines_date       ON purchase_lines (date_invoiced DESC);
CREATE INDEX IF NOT EXISTS idx_purchase_lines_supplier   ON purchase_lines (supplier);
CREATE INDEX IF NOT EXISTS idx_purchase_lines_category   ON purchase_lines (category);
CREATE INDEX IF NOT EXISTS idx_purchase_lines_part_no    ON purchase_lines (part_no);
CREATE INDEX IF NOT EXISTS idx_purchase_lines_po_number  ON purchase_lines (po_number);

-- Trigram index for fast LIKE searches on description (e.g. "%cable%")
CREATE INDEX IF NOT EXISTS idx_purchase_lines_desc_trgm
  ON purchase_lines USING gin (description gin_trgm_ops);

-- ----------------------------------------------------------------------------
-- sync_runs: log of every sync attempt (for debugging + monitoring)
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS sync_runs (
  id                BIGSERIAL PRIMARY KEY,
  started_at        TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  finished_at       TIMESTAMPTZ,
  status            TEXT NOT NULL DEFAULT 'running',  -- 'running' | 'ok' | 'failed'
  trigger           TEXT,                              -- 'cron' | 'manual'
  purchase_orders   INT DEFAULT 0,
  lines_inserted    INT DEFAULT 0,
  lines_updated     INT DEFAULT 0,
  duration_ms       INT,
  error             TEXT
);

CREATE INDEX IF NOT EXISTS idx_sync_runs_started ON sync_runs (started_at DESC);
