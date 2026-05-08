// index.js — main API entry point

const express = require('express');
const db = require('./db');

const app = express();
app.use(express.json());

const PORT = process.env.PORT || 3000;

// --- Health check ---------------------------------------------------------
// Hit this endpoint to confirm the server is up and the DB is reachable.
app.get('/health', async (req, res) => {
  try {
    const result = await db.query('SELECT NOW() as server_time');
    res.json({
      status: 'ok',
      database: 'connected',
      server_time: result.rows[0].server_time,
    });
  } catch (err) {
    console.error('Health check failed:', err);
    res.status(500).json({
      status: 'error',
      database: 'disconnected',
      message: err.message,
    });
  }
});

// --- Example resource: items ---------------------------------------------
// Replace "items" with whatever your app actually deals with later.

// List all items
app.get('/items', async (req, res) => {
  try {
    const result = await db.query('SELECT * FROM items ORDER BY id DESC');
    res.json(result.rows);
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: err.message });
  }
});

// Get one item
app.get('/items/:id', async (req, res) => {
  try {
    const result = await db.query('SELECT * FROM items WHERE id = $1', [req.params.id]);
    if (result.rows.length === 0) {
      return res.status(404).json({ error: 'Not found' });
    }
    res.json(result.rows[0]);
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: err.message });
  }
});

// Create an item
app.post('/items', async (req, res) => {
  const { name, description } = req.body;
  if (!name) {
    return res.status(400).json({ error: 'name is required' });
  }
  try {
    const result = await db.query(
      'INSERT INTO items (name, description) VALUES ($1, $2) RETURNING *',
      [name, description || null]
    );
    res.status(201).json(result.rows[0]);
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: err.message });
  }
});

// Delete an item
app.delete('/items/:id', async (req, res) => {
  try {
    const result = await db.query('DELETE FROM items WHERE id = $1 RETURNING *', [req.params.id]);
    if (result.rows.length === 0) {
      return res.status(404).json({ error: 'Not found' });
    }
    res.json({ deleted: result.rows[0] });
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: err.message });
  }
});

// --- AroFlo test endpoint -------------------------------------------------
// Quick ping to confirm Railway can reach api.aroflo.com and our credentials work.
// Visit: /test-aroflo                       (defaults to /purchaseorders)
//        /test-aroflo?path=/purchaseorders  (override the AroFlo path)
//
// IMPORTANT: this endpoint will be removed once the real sync is built.
app.get('/test-aroflo', async (req, res) => {
  const u = process.env.AROFLO_U_ENCODED;
  const p = process.env.AROFLO_P_ENCODED;
  const org = process.env.AROFLO_ORG_ENCODED;

  if (!u || !p || !org) {
    return res.status(500).json({
      error: 'AroFlo credentials not set in Railway environment variables.',
      missing: {
        AROFLO_U_ENCODED: !u,
        AROFLO_P_ENCODED: !p,
        AROFLO_ORG_ENCODED: !org,
      },
    });
  }

  const path = req.query.path || '/purchaseorders';
  const url = `https://api.aroflo.com${path}?uencoded=${encodeURIComponent(u)}&pencoded=${encodeURIComponent(p)}&orgencoded=${encodeURIComponent(org)}`;

  try {
    const response = await fetch(url, { method: 'GET' });
    const contentType = response.headers.get('content-type') || '';
    const status = response.status;
    let body;
    if (contentType.includes('application/json')) {
      body = await response.json();
    } else {
      body = await response.text();
    }
    // Redact credentials from any URL we echo back
    const safeUrl = `https://api.aroflo.com${path}?uencoded=***&pencoded=***&orgencoded=***`;
    res.json({
      status,
      contentType,
      url: safeUrl,
      body,
    });
  } catch (err) {
    console.error('AroFlo test failed:', err);
    res.status(500).json({ error: err.message });
  }
});

// --- Start server ---------------------------------------------------------
app.listen(PORT, () => {
  console.log(`API listening on port ${PORT}`);
});
