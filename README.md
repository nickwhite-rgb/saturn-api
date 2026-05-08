# Saturn API — Node.js + Railway Postgres

A starter Express API that connects to your Railway Postgres database. Deployed by pushing to GitHub and connecting the repo to Railway.

## Endpoints

- `GET  /health`       — health check (also confirms DB connection)
- `GET  /items`        — list all items
- `GET  /items/:id`    — get one item
- `POST /items`        — create an item
- `DELETE /items/:id`  — delete an item

## Files

- `index.js` — Express server with health check + example CRUD endpoints
- `db.js` — Postgres connection pool (reads `DATABASE_URL` from env)
- `schema.sql` — SQL to create the example `items` table
- `package.json` — dependencies + start script
- `.gitignore` — keeps `node_modules` and `.env` out of git
- `.env.example` — template for local testing
