#!/usr/bin/env sh
# Download the SQLite amalgamation into vendor/sqlite/ (sqlite3.c + headers).
set -eu
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DEST="$ROOT/vendor/sqlite"
ZIP_URL="${SQLITE_AMALG_URL:-https://www.sqlite.org/2025/sqlite-amalgamation-3490200.zip}"

if [ -f "$DEST/sqlite3.c" ] && [ -f "$DEST/sqlite3.h" ]; then
  exit 0
fi

mkdir -p "$DEST"
tmpdir="$(mktemp -d)"
cleanup() { rm -rf "$tmpdir"; }
trap cleanup EXIT

curl -fsSL -o "$tmpdir/sa.zip" "$ZIP_URL"
unzip -o -j "$tmpdir/sa.zip" "*/sqlite3.c" "*/sqlite3.h" "*/sqlite3ext.h" -d "$DEST"
echo "Fetched SQLite amalgamation into $DEST"
