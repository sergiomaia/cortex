-- Minimal Cortex test fixture seed (SQLite, in-memory callers may apply via db_connection_exec).

CREATE TABLE IF NOT EXISTS posts (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  title TEXT NOT NULL,
  body TEXT
);

INSERT INTO posts (title, body) VALUES ('fixture', 'seed row');
