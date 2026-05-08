-- schema.sql
-- Run this once in Railway's database query tool to create the example table.

CREATE TABLE IF NOT EXISTS items (
  id SERIAL PRIMARY KEY,
  name TEXT NOT NULL,
  description TEXT,
  created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

-- Optional: a couple of test rows so /items returns something
INSERT INTO items (name, description) VALUES
  ('First item', 'Hello from Railway Postgres'),
  ('Second item', 'It works!');
