// import.js — bulk CSV import for historical purchase data
//
// Maps Nick's AroFlo custom report CSV columns to our schema.
// Generates a deterministic synthetic line_id so re-importing the same CSV
// upserts cleanly (no duplicates).

const crypto = require('crypto');
const { parse } = require('csv-parse/sync');
const db = require('./db');

// Column header -> our internal field. Keep keys exactly as they appear in CSV header row.
const COLUMN_MAP = {
  'Purchased Date':       'date_invoiced',
  'Orderno':              'po_number',
  'Invoice Number':       'supplier_invoice_no',
  'Supplier Name':        'supplier',
  'Item Part No':         'part_no',
  'Bill Item Description':'description',
  'Item Qty Received':    'qty',
  'Item Cost Ex':         'unit_price',
  'Item Total Ex':        'line_total',
};

// --- helpers -----------------------------------------------------------------

function safeText(v) {
  if (v === null || v === undefined) return null;
  const s = String(v).trim();
  return s.length === 0 ? null : s;
}

function safeNumber(v) {
  const t = safeText(v);
  if (t === null) return null;
  // Strip $ , whitespace
  const cleaned = t.replace(/[$,\s]/g, '');
  const n = parseFloat(cleaned);
  return Number.isNaN(n) ? null : n;
}

// Convert "9/4/2026" (DD/MM/YYYY) or "2026/04/09" -> ISO date string
function safeDate(v) {
  const t = safeText(v);
  if (!t) return null;

  // Try DD/MM/YYYY (Australian)
  const auMatch = t.match(/^(\d{1,2})\/(\d{1,2})\/(\d{4})/);
  if (auMatch) {
    const [, d, m, y] = auMatch;
    const iso = `${y}-${String(m).padStart(2, '0')}-${String(d).padStart(2, '0')}`;
    const parsed = new Date(iso + 'T00:00:00');
    if (!Number.isNaN(parsed.getTime())) return parsed.toISOString();
  }

  // Try YYYY/MM/DD
  const ymMatch = t.match(/^(\d{4})\/(\d{1,2})\/(\d{1,2})/);
  if (ymMatch) {
    const [, y, m, d] = ymMatch;
    const iso = `${y}-${String(m).padStart(2, '0')}-${String(d).padStart(2, '0')}`;
    const parsed = new Date(iso + 'T00:00:00');
    if (!Number.isNaN(parsed.getTime())) return parsed.toISOString();
  }

  // Last-ditch: native Date parse
  const fallback = new Date(t);
  return Number.isNaN(fallback.getTime()) ? null : fallback.toISOString();
}

// Build a stable, deterministic ID from the row contents so re-imports upsert
function syntheticLineId(row) {
  const composite = [
    row.date_invoiced || '',
    row.po_number || '',
    row.part_no || '',
    row.qty != null ? String(row.qty) : '',
    row.unit_price != null ? String(row.unit_price) : '',
    row.line_total != null ? String(row.line_total) : '',
    row.supplier_invoice_no || '',
  ].join('|');
  const hash = crypto.createHash('sha1').update(composite).digest('hex');
  return `csv:${hash.slice(0, 24)}`;
}

// --- main entry --------------------------------------------------------------

async function importCsv(csvText) {
  const start = Date.now();

  // Open a sync_runs record (yes, we use the same table for visibility)
  const runRow = await db.query(
    `INSERT INTO sync_runs (status, trigger) VALUES ('running', 'csv-import') RETURNING id`
  );
  const runId = runRow.rows[0].id;

  let inserted = 0;
  let updated = 0;
  let skipped = 0;
  const headerWarnings = [];

  try {
    const records = parse(csvText, {
      columns: true,
      skip_empty_lines: true,
      trim: true,
      bom: true,            // handle UTF-8 BOM if present
      relax_quotes: true,
      relax_column_count: true, // accept summary/total rows at end of file
    });

    if (records.length === 0) {
      throw new Error('CSV had a header row but no data rows');
    }

    // Verify expected columns exist (warn, don't fail, on missing)
    const headers = Object.keys(records[0]);
    for (const expected of Object.keys(COLUMN_MAP)) {
      if (!headers.includes(expected)) {
        headerWarnings.push(`Missing expected column: "${expected}"`);
      }
    }

    for (const raw of records) {
      const row = {
        date_invoiced:       safeDate(raw[Object.keys(COLUMN_MAP).find(k => COLUMN_MAP[k] === 'date_invoiced')]),
        po_number:           safeText(raw[Object.keys(COLUMN_MAP).find(k => COLUMN_MAP[k] === 'po_number')]),
        supplier_invoice_no: safeText(raw[Object.keys(COLUMN_MAP).find(k => COLUMN_MAP[k] === 'supplier_invoice_no')]),
        supplier:            safeText(raw[Object.keys(COLUMN_MAP).find(k => COLUMN_MAP[k] === 'supplier')]),
        part_no:             safeText(raw[Object.keys(COLUMN_MAP).find(k => COLUMN_MAP[k] === 'part_no')]),
        description:         safeText(raw[Object.keys(COLUMN_MAP).find(k => COLUMN_MAP[k] === 'description')]),
        qty:                 safeNumber(raw[Object.keys(COLUMN_MAP).find(k => COLUMN_MAP[k] === 'qty')]),
        unit_price:          safeNumber(raw[Object.keys(COLUMN_MAP).find(k => COLUMN_MAP[k] === 'unit_price')]),
        line_total:          safeNumber(raw[Object.keys(COLUMN_MAP).find(k => COLUMN_MAP[k] === 'line_total')]),
      };

      // Skip rows with no PO number — likely a junk/total/blank row
      if (!row.po_number) {
        skipped++;
        continue;
      }

      // Skip empty/cancelled POs (no part info AND zero qty/price)
      const hasPartInfo = row.part_no || row.description;
      const hasValue = (row.qty && row.qty > 0) || (row.unit_price && row.unit_price > 0);
      if (!hasPartInfo && !hasValue) {
        skipped++;
        continue;
      }

      // Skip "Total = ..." summary rows that AroFlo appends to the bottom
      if (row.po_number && /^Total\s*=/i.test(row.po_number)) {
        skipped++;
        continue;
      }

      const lineId = syntheticLineId(row);

      const result = await db.query(
        `INSERT INTO purchase_lines (
           line_id, date_invoiced, po_number, supplier_invoice_no, supplier,
           part_no, description, category, qty, unit_price, line_total,
           source, synced_at
         ) VALUES (
           $1, $2, $3, $4, $5, $6, $7, NULL, $8, $9, $10, 'csv', NOW()
         )
         ON CONFLICT (line_id) DO UPDATE SET
           date_invoiced       = EXCLUDED.date_invoiced,
           po_number           = EXCLUDED.po_number,
           supplier_invoice_no = EXCLUDED.supplier_invoice_no,
           supplier            = EXCLUDED.supplier,
           part_no             = EXCLUDED.part_no,
           description         = EXCLUDED.description,
           qty                 = EXCLUDED.qty,
           unit_price          = EXCLUDED.unit_price,
           line_total          = EXCLUDED.line_total,
           source              = 'csv',
           synced_at           = NOW()
         RETURNING (xmax = 0) AS inserted`,
        [
          lineId, row.date_invoiced, row.po_number, row.supplier_invoice_no, row.supplier,
          row.part_no, row.description, row.qty, row.unit_price, row.line_total,
        ]
      );
      if (result.rows[0].inserted) inserted++;
      else updated++;
    }

    const durationMs = Date.now() - start;
    await db.query(
      `UPDATE sync_runs
         SET status='ok', finished_at=NOW(),
             purchase_orders=$1, lines_inserted=$2, lines_updated=$3, duration_ms=$4
       WHERE id=$5`,
      [records.length, inserted, updated, durationMs, runId]
    );

    return {
      ok: true,
      runId,
      durationMs,
      totalRows: records.length,
      inserted,
      updated,
      skipped,
      headerWarnings,
    };
  } catch (err) {
    console.error('[CSV IMPORT] failed:', err);
    const durationMs = Date.now() - start;
    await db.query(
      `UPDATE sync_runs
         SET status='failed', finished_at=NOW(), duration_ms=$1, error=$2
       WHERE id=$3`,
      [durationMs, err.message, runId]
    );
    throw err;
  }
}

module.exports = { importCsv, COLUMN_MAP };
