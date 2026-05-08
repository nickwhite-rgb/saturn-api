// sync.js — pull purchase orders from AroFlo and upsert into Postgres
//
// Called from:
//   - the manual trigger endpoint /sync/run
//   - the scheduled cron jobs at 6am + 1pm Brisbane

const db = require('./db');
const aroflo = require('./aroflo');

// --- helpers -----------------------------------------------------------------

function safeText(v) {
  if (v === null || v === undefined) return null;
  if (typeof v === 'object') {
    // empty XML element parses to {}
    if (Object.keys(v).length === 0) return null;
    return null;
  }
  const s = String(v).trim();
  return s.length === 0 ? null : s;
}

function safeNumber(v) {
  const t = safeText(v);
  if (t === null) return null;
  const n = parseFloat(t);
  return Number.isNaN(n) ? null : n;
}

// AroFlo dates come as "2026/04/09" or "2026/04/09 10:30:00"
function safeDate(v) {
  const t = safeText(v);
  if (!t) return null;
  const cleaned = t.replace(/\//g, '-');
  const d = new Date(cleaned);
  return Number.isNaN(d.getTime()) ? null : d.toISOString();
}

function arrayify(v) {
  if (v === null || v === undefined) return [];
  return Array.isArray(v) ? v : [v];
}

// --- main sync ---------------------------------------------------------------

async function syncPurchaseOrders({ trigger = 'manual' } = {}) {
  const start = Date.now();

  // Open a sync_runs record
  const runRow = await db.query(
    `INSERT INTO sync_runs (status, trigger) VALUES ('running', $1) RETURNING id`,
    [trigger]
  );
  const runId = runRow.rows[0].id;

  let inserted = 0;
  let updated = 0;
  let purchaseOrders = [];

  try {
    purchaseOrders = await aroflo.fetchAllPages({
      zone: 'purchaseorders',
      join: 'lineitems',
    });
    console.log(`[SYNC] fetched ${purchaseOrders.length} POs from AroFlo`);

    for (const po of purchaseOrders) {
      const supplierName     = safeText(po.supplier && po.supplier.orgname);
      const supplierOrgId    = safeText(po.supplier && po.supplier.orgid);
      const supplierInvoice  = safeText(po.supplierinvoicenumber);
      const poNumber         = safeText(po.ordernumber);
      const purchaseOrderId  = safeText(po.purchaseorderid);
      const status           = safeText(po.status);
      const dateInvoiced     = safeDate(po.dateinvoiced) || safeDate(po.purchasedate);

      const lines = arrayify(po.lines && po.lines.line);
      for (const line of lines) {
        const lineId = safeText(line.lineid);
        if (!lineId) {
          console.warn('[SYNC] line missing lineid, skipping');
          continue;
        }

        const partNo      = safeText(line.partno);
        const description = safeText(line.description) || safeText(line.item);
        const category    = safeText(line.category); // may be null until we wire item lookup
        const qty         = safeNumber(line.qtyordered) || safeNumber(line.qtybilled);
        const unitPrice   = safeNumber(line.price) || safeNumber(line.cost);
        const lineTotal   = safeNumber(line.total);
        const taskId      = safeText(line.taskid);

        const result = await db.query(
          `INSERT INTO purchase_lines (
             line_id, date_invoiced, po_number, supplier_invoice_no, supplier,
             part_no, description, category, qty, unit_price, line_total,
             purchase_order_id, supplier_org_id, task_id, status, synced_at
           ) VALUES (
             $1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13, $14, $15, NOW()
           )
           ON CONFLICT (line_id) DO UPDATE SET
             date_invoiced       = EXCLUDED.date_invoiced,
             po_number           = EXCLUDED.po_number,
             supplier_invoice_no = EXCLUDED.supplier_invoice_no,
             supplier            = EXCLUDED.supplier,
             part_no             = EXCLUDED.part_no,
             description         = EXCLUDED.description,
             category            = EXCLUDED.category,
             qty                 = EXCLUDED.qty,
             unit_price          = EXCLUDED.unit_price,
             line_total          = EXCLUDED.line_total,
             purchase_order_id   = EXCLUDED.purchase_order_id,
             supplier_org_id     = EXCLUDED.supplier_org_id,
             task_id             = EXCLUDED.task_id,
             status              = EXCLUDED.status,
             synced_at           = NOW()
           RETURNING (xmax = 0) AS inserted`,
          [
            lineId, dateInvoiced, poNumber, supplierInvoice, supplierName,
            partNo, description, category, qty, unitPrice, lineTotal,
            purchaseOrderId, supplierOrgId, taskId, status,
          ]
        );
        if (result.rows[0].inserted) inserted++;
        else updated++;
      }
    }

    const durationMs = Date.now() - start;
    await db.query(
      `UPDATE sync_runs
         SET status='ok', finished_at=NOW(),
             purchase_orders=$1, lines_inserted=$2, lines_updated=$3, duration_ms=$4
       WHERE id=$5`,
      [purchaseOrders.length, inserted, updated, durationMs, runId]
    );

    return {
      ok: true,
      runId,
      durationMs,
      purchaseOrders: purchaseOrders.length,
      inserted,
      updated,
    };
  } catch (err) {
    console.error('[SYNC] failed:', err);
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

module.exports = { syncPurchaseOrders };
