// db.js — Postgres connection pool
// Reads DATABASE_URL from environment variables.
// On Railway, DATABASE_URL is injected automatically when this service
// is in the same project as the Postgres service (via a Variable Reference).

const { Pool } = require('pg');

if (!process.env.DATABASE_URL) {
  console.error('ERROR: DATABASE_URL environment variable is not set.');
  console.error('On Railway, link this service to the Postgres service via Variable Reference.');
  process.exit(1);
}

const pool = new Pool({
  connectionString: process.env.DATABASE_URL,
  // Railway internal connections do not need SSL.
  // If you ever switch to DATABASE_PUBLIC_URL, you'll need: ssl: { rejectUnauthorized: false }
});

pool.on('error', (err) => {
  console.error('Unexpected error on idle Postgres client', err);
  process.exit(-1);
});

module.exports = {
  query: (text, params) => pool.query(text, params),
  pool,
};
