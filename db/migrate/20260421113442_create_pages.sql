-- Migration: 20260421113442_create_pages

-- migrate:up
CREATE TABLE pages (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  title TEXT,
  created_at DATETIME,
  updated_at DATETIME
);

-- migrate:down
DROP TABLE pages;
