# Welcome to Cortex

Cortex is a web application framework designed to feel familiar if you've
used Ruby on Rails, but implemented in C and oriented toward AI-native
workflows.

It is organized in layers and kept intentionally small so each responsibility
can be built and extended incrementally.

## What's Cortex?

Cortex is a web application framework that includes the building blocks to
create database-backed web applications using the
[Model-View-Controller (MVC)](https://en.wikipedia.org/wiki/Model-view-controller)
pattern.

MVC divides an application into three layers:

1. **Model**: domain data and business logic.
2. **View**: presentation templates (HTML or other representations).
3. **Controller**: request handling and response building.

In Cortex, those layers map to:

- **Model layer** -> `Active` (data + ORM-like primitives)
- **View layer** -> `Action View` (rendering templates into responses)
- **Controller layer** -> `Action` (HTTP controllers + request/response)

## Model layer

The **Model layer** represents the domain model (for example: `Account`,
`Product`, `Person`, `Post`, etc.) and encapsulates application-specific
business logic.

In Cortex, the data layer is implemented via the `Active` module:

- `ActiveModel`-style abstractions for declaring fields and updating model
  state.
- `ActiveRecord`-style primitives for storing data in a backing store.
- migration helpers (via the `Active Migration` primitives) to evolve storage
  layout.

When you generate a scaffold, Cortex creates a corresponding model under
`app/models/` that wires your model type into the `Active` primitives.

## View layer

The **View layer** is composed of templates responsible for producing the
representation returned to the client.

Cortex supports two complementary view modes:

1. **Static HTML** (`app/views/**/*.html`) — loaded at runtime by `render_view()`
   and `render_html()` (unchanged, backward compatible for existing apps).
2. **Compiled templates** (`app/views/**/*.chtml`) — ERB-style syntax compiled to
   pure C at build time (zero parsing at runtime). **Forge generators** (`cortex new`,
   `generate scaffold`, `generate resource`) create `.chtml` views and controllers
   that call `action_view_render()` via the generated `forge_render_chtml()` helper.

New projects include a `Makefile` with `CHTML_COMPILE`, `make views`, and link rules
for `build/chtml/**/*.chtml.c`.

---

## Action View — compiled `.chtml` templates

### Overview

`.chtml` templates use an ERB-like syntax. At **build time**, the
`chtml_compile` tool converts each template into a C function. There is **no
template parser at runtime** — performance is equivalent to hand-written C
that calls `cx_write()` / `cx_write_escaped()`.

```
app/views/posts/index.chtml
        ↓  (make / chtml_compile)
build/chtml/app/views/posts/index.chtml.c
        ↓  (gcc)
view_posts_index(CxContext *cx)  +  CORTEX_VIEW("posts/index", …)
        ↓
action_view_render("posts/index", &cx, html, sizeof(html))
        ↓  (optional layout)
HTML in `out`
```

### Syntax (ERB-compatible)

| Tag | Meaning |
|-----|---------|
| `<%= expr %>` | Evaluate C expression, **HTML-escape** output (`cx_write_escaped`) |
| `<%== expr %>` | Evaluate C expression, **raw** output (`cx_write`) |
| `<% stmt %>` | Emit C statement(s) (control flow, loops, etc.) |
| `<%# comment %>` | Comment — removed at compile time |
| Plain text | Emitted as `cx_write(cx, "…", N)` |

**Example** — `app/views/posts/index.chtml`:

```html
<h1>Posts (<%= cx_get(cx, "posts_count") %>)</h1>

<% if (cx_get_int(cx, "posts_count") == 0) { %>
  <p>Nenhum post encontrado.</p>
<% } else { %>
  <ul>
    <%== cx_get(cx, "posts_list") %>
  </ul>
<% } %>
```

Expressions must be valid **C**. Use `cx_get(cx, "key")`, `cx_get_int(cx, "key")`,
and `cx_isset(cx, "key")` for template variables.

### `CxContext` — render context

Headers: `action/cx_context.h`, implementation: `action/cx_context.c`.

```c
CxContext cx;
cx_init(&cx);

cx_set(&cx, "page_title", "Posts");
cx_set_int(&cx, "posts_count", 0);
cx_set_fmt(&cx, "label", "item-%d", 7);
cx_set_layout(&cx, "application");   /* default; see layouts */
/* cx_set_layout(&cx, NULL); */      /* disable layout */

char html[64 * 1024];
if (action_view_render("posts/index", &cx, html, (int)sizeof(html)) != 0) {
    /* handle error */
}
```

| API | Role |
|-----|------|
| `cx_write` / `cx_writef` | Append literal or formatted text |
| `cx_write_escaped` / `cx_writef_escaped` | Append with HTML escape (`& " ' < >`) |
| `cx_set` / `cx_set_int` / `cx_set_fmt` | Template variables |
| `cx_get` / `cx_get_int` / `cx_isset` | Read variables in templates |
| `cx_set_layout` | Layout name (default `"application"`) or `NULL` |
| `cx_yield` | Yield buffer content (for layouts) |

Buffers are bounded (`CX_BUF_SIZE` = 256 KiB); all writes are overflow-safe and
null-terminated.

### Layouts and yield

Layouts live under `app/views/layouts/*.chtml` and register as
`layouts/<name>` (e.g. `layouts/application`).

Render flow:

1. Render the view into an internal buffer.
2. Copy result to `yield_buf`, clear main buffer.
3. If `cx->layout` is set, render `layouts/<layout>`.
4. Layout inserts the view via `<%== cx_yield(cx) %>`.
5. Copy final HTML to the caller's `out` buffer.

**Example** — `app/views/layouts/application.chtml` (excerpt):

```html
<title><%= cx_get(cx, "page_title") %> — <%= cx_get(cx, "app_name") %></title>
<main>
  <%== cx_yield(cx) %>
</main>
```

### Controller integration

```c
#include "action_view.h"
#include "cx_context.h"

void posts_index(ActionRequest *req, ActionResponse *res) {
    CxContext cx;
    char html[256 * 1024];
    (void)req;

    cx_init(&cx);
    cx_set_int(&cx, "posts_count", 12);
    cx_set(&cx, "page_title", "Posts");
    cx_set(&cx, "app_name", "MeuApp");

    if (action_view_render("posts/index", &cx, html, (int)sizeof(html)) != 0) {
        action_response_set(res, 500, "Template render error");
        return;
    }
    render_html(res, strdup(html));  /* layout + JS injection for HTTP */
}
```

`render_view()` / `render_html()` for static `.html` files are **unchanged**.

### Build integration (`Makefile`)

| Target / variable | Purpose |
|-------------------|---------|
| `make` / `make all` | Build compiler, compile templates, link `cortex` |
| `make views` | Compile all `app/views/**/*.chtml` only |
| `CHTML_COMPILE` | `build/chtml_compile` |
| Generated sources | `build/chtml/app/views/.../*.chtml.c` |
| Generated objects | Linked into `cortex` and tests (not only `libcortex.a`) |

Compiler CLI:

```bash
build/chtml_compile input.chtml output.chtml.c function_name view_name
```

Example:

```bash
build/chtml_compile app/views/posts/index.chtml \
  build/chtml/app/views/posts/index.chtml.c \
  view_posts_index posts/index
```

Auto-registration (each generated `.c` file ends with):

```c
CORTEX_VIEW("posts/index", view_posts_index)
```

View names mirror paths under `app/views/` (without `.chtml`). Function names:
`view_<path_with_underscores>`.

### Security and performance

- **HTML escape** on `<%=` via `cx_write_escaped` (`&`, `"`, `'`, `<`, `>`).
- **No runtime parsing** — invalid templates fail at compile time with file/line errors.
- **No external dependencies** — compiler and runtime are C11 only.
- **Incremental builds** — `make` recompiles only changed `.chtml` files.
- **Static HTML still works** — no migration required for existing `.html` views.

### Differences from Rails ERB

| Rails ERB | Cortex `.chtml` |
|-----------|-----------------|
| Ruby expressions | **C expressions** |
| Runtime interpretation | **Build-time compilation** |
| Instance variables (`@foo`) | `cx_set` / `cx_get(cx, "foo")` |
| `yield` in layouts | `cx_yield(cx)` |
| `html_safe` / `raw` | `<%== ... %>` |

### Troubleshooting

| Symptom | Likely cause |
|---------|----------------|
| `action_view_render` returns `-2` | View not registered — ensure `.chtml.o` objects are linked (see Makefile) |
| Returns `-5` | Layout missing — add `app/views/layouts/<name>.chtml` or `cx_set_layout(cx, NULL)` |
| `chtml_compile: unclosed tag` | Missing `%>` or `<%` inside literal confused with tag — escape or split literals |
| Truncated output | `out` buffer too small — increase size or check return `-6` |
| Empty variable in template | Key not set — use `cx_isset` before `cx_get` |

### Tests

Template tests live in:

- `tests/action/test_cx_context.c`
- `tests/action/test_action_chtml.c`
- `tests/forge/test_chtml_compile.c`
- `tests/fixtures/views/**/*.chtml`

Run: `make test`

## Controller layer

The **Controller layer** is responsible for handling incoming HTTP requests and
providing an appropriate response.

In Cortex:

- incoming requests are routed by `ActionRouter` (configured via
  `config/routes.c`).
- dispatch is performed by `action_dispatch`, which matches a handler for the
  request `method` and `path`.
- controllers are functions that accept an `ActionRequest` and populate an
  `ActionResponse`.

The default scaffold includes routes for:

- `GET /` (home)
- `GET /health` (basic server health check)

## Frameworks and libraries

Rails organizes features into independent libraries. Cortex follows the same
spirit: each major module can be understood as a separate “framework-like”
area that plugs into the overall request lifecycle.

In addition to `Action` (controllers/dispatch) and `Active` (models/data),
Cortex includes:

- **Core**: runtime, config, logging, and utilities
- **Neural**: AI/LLM models, prompts, memory, retrieval, agents, streaming
- **Flow**: jobs, queues, workers, pipelines
- **Forge**: project and code generators
- **Guard**: authentication, policies, and security helpers
- **Pulse**: structured logging (TEXT + NDJSON), metrics, tracing, AI observability
- **Cache**: in-memory and external cache abstractions
- **CLI**: the `cortex` command entrypoint
- **App**: example application code (controllers, models, neural)
- **DB**: engine-agnostic `db_connection` API; **SQLite** is the default backend (`db/sqlite/sqlite_adapter.c`). **PostgreSQL** via **libpq** is optional at build time (`db/postgres/postgres_adapter.c` when `pkg-config libpq` succeeds).

## Database

Cortex uses **SQLite as the default database**, in the spirit of Rails: **zero YAML**, **convention over configuration**, and a project that runs after `cortex new` without manual DB setup. **PostgreSQL is fully optional**: unless you opt in with environment variables, behavior matches the historical SQLite-only workflow.

### SQLite (default)

No extra system libraries are required beyond the bundled SQLite amalgamation. The `make` output line `[cortex] PostgreSQL adapter disabled` indicates a normal SQLite-only build when libpq is not installed.

### Convention (paths and environment)

| `CORE_ENV`    | Database file              |
|---------------|----------------------------|
| `development` | `db/development.sqlite3`   |
| `test`        | `db/test.sqlite3`         |
| `production`  | `db/production.sqlite3`    |

- `CORE_ENV` defaults to **`development`** when unset (same as `core_config_load()`).
- The path helper is `db_path_for_environment()` in `db/db_paths.c` / `db/db_paths.h`.

### Architecture

- **`db/db_connection.h`** — Engine-agnostic API: `db_connection_open` (SQLite file path), `exec`, prepared statements (`prepare` / `step` / `bind_*` / `column_*` / `finalize`), `db_connection_close`. Use **`db_connection_backend()`** to distinguish SQLite vs PostgreSQL when implementing portability shims (migrations do this internally).
- **`db/sqlite/sqlite_adapter.c`** — Default implementation: SQLite via `sqlite3.h`, registered through the internal vtable used by `db_connection.c`.
- **`db/postgres/postgres_adapter.h`** (when built with `-DCORTEX_HAVE_POSTGRES`) — **`postgres_connect(const char *connstring)`**, **`postgres_connect_from_env()`** for pool/bootstrap. Prepared statements accept Cortex’s SQLite-style **`?` placeholders**; the adapter rewrites them to PostgreSQL **`$1`…`$n`**.
- **`db/db_bootstrap.h`** — Application lifecycle:
  - **`cortex_db_env_wants_postgresql()`** — Returns whether **`DATABASE_URL`** (scheme `postgres` / `postgresql`) or **`DB_ADAPTER=postgresql`** selects PostgreSQL for this process.
  - **`cortex_db_bootstrap()`** — Runs migrations automatically in **non-production** (`CORE_ENV` unset, `development`, `test`, etc.): SQLite via **`db_migrate_default()`**, PostgreSQL via **`db_migrate_default_on_connection()`** on a short-lived probe connection. In **production**, pending migrations are **not** applied; a **`LOG_WARN`** entry reminds you to run **`cortex db:migrate`**, then **`cortex_db_init()`** opens the pool. On hard failure the thread-local Cortex error is set.
  - **`cortex_db_shutdown()`** — Closes pool connections on exit.
  - **`cortex_db_exec(const char *sql)`** — Executes SQL on a pooled connection.

### PostgreSQL (optional)

PostgreSQL support is compiled in **only** when **`pkg-config --exists libpq`** succeeds. The Makefile adds `-DCORTEX_HAVE_POSTGRES`, `libpq` CFLAGS/LDFLAGS, and links **`-lpq`**. If libpq is missing, Cortex builds exactly as before (SQLite only); you will see `[cortex] PostgreSQL adapter disabled — libpq not found`.

#### Installing libpq

**Ubuntu / Debian**

```bash
sudo apt-get install libpq-dev
```

**macOS (Homebrew)**

```bash
brew install libpq
```

Follow Homebrew’s notes to expose `pkg-config` and `libpq` on your `PATH` if `pkg-config libpq` does not resolve.

#### Opt-in environment variables

| Variable | Role |
|----------|------|
| **`DB_ADAPTER`** | Set to **`postgresql`** to select PostgreSQL when no `DATABASE_URL` is used. |
| **`DATABASE_URL`** | If the URL starts with **`postgres`** or **`postgresql`**, Cortex uses PostgreSQL (libpq connection string / URI). |
| **`PGHOST`**, **`PGPORT`**, **`PGDATABASE`**, **`PGUSER`**, **`PGPASSWORD`** | Standard libpq variables; used by **`PQconnectdb("")`** and when building a connection from env. |
| **`CORE_ENV`** | When **`PGDATABASE`** is **not** set, **`postgres_connect_from_env()`** derives a database name **`${app}_${CORE_ENV}`** (e.g. `cortex_development`), where **`app`** is sanitized from the current working directory basename. **`CORE_ENV`** defaults to **`development`**. |

SQLite path conventions (`db/<env>.sqlite3`) are unchanged when PostgreSQL is not selected.

#### Usage examples

**Adapter + libpq env (no `DATABASE_URL`):**

```bash
export DB_ADAPTER=postgresql
export PGHOST=localhost
export PGUSER=postgres
export PGPASSWORD=secret
make server
```

**Connection URI:**

```bash
DATABASE_URL=postgresql://postgres:secret@localhost:5432/blog_development make server
```

**Run tests against PostgreSQL** (optional integration — skipped if unset or unreachable):

```bash
CORE_ENV=test DB_ADAPTER=postgresql CORTEX_TEST_PG=1 make test
```

Ensure the target database exists and credentials match. Tests that need a live server **noop** when `CORTEX_TEST_PG` is unset so CI stays stable.

#### Migration portability

Include **`active/active_migration.h`** (wraps **`core/active_migration.h`**) for portable SQL fragments:

| Macro | SQLite | PostgreSQL |
|-------|--------|------------|
| **`SQL_PK`** | `INTEGER PRIMARY KEY AUTOINCREMENT` | `SERIAL PRIMARY KEY` |
| **`SQL_NOW`** | `CURRENT_TIMESTAMP` | `NOW()` |
| **`SQL_BOOL`** | `INTEGER` | `BOOLEAN` |
| **`SQL_DATETIME`** | `TEXT` | `TIMESTAMPTZ` |

Example:

```c
db_connection_exec(db,
    "CREATE TABLE posts ("
    "  id " SQL_PK ","
    "  title VARCHAR(255) NOT NULL,"
    "  created_at " SQL_DATETIME " DEFAULT " SQL_NOW
    ");"
);
```

**`db_schema_write_dump()`** (SQLite `sqlite_master` dump) is a no-op on PostgreSQL so `cortex db:migrate` does not fail when mixing backends.

#### Backend overview

```
DbConnection / DbStatement  →  db_connection_*() API
         ↓
   SQLite adapter  |  PostgreSQL adapter (optional)
   sqlite3.c       |  libpq
```

Models, controllers, and Active helpers use **`db_connection_*`** only; they do not depend on a specific engine unless you branch on **`db_connection_backend()`** for maintenance tasks.

### Migrations

Cortex supports **two complementary migration mechanisms** (SQL files are primary; C callbacks remain for backward compatibility):

1. **SQL file migrations (Rails-style, recommended)** — files under **`db/migrate/`** named **`YYYYMMDDHHMMSS_snake_case_description.sql`**. Each file uses section markers:
   - **`-- migrate:up`** (required): SQL applied when migrating forward.
   - **`-- migrate:down`** (optional): SQL applied on rollback; if omitted, `cortex db:rollback` logs a warning and skips that version.
   - Text before the first marker is ignored. Multiple statements are supported; the runner splits on `;` outside quotes, line comments (`--`), block comments (`/* */`), escaped quotes, and PostgreSQL **dollar-quoted** strings. **Limitation:** semicolons inside unquoted PL/pgSQL bodies can still split incorrectly—use dollar-quoting for functions and triggers.
   - Override the directory with **`CORTEX_MIGRATE_DIR`**.
2. **C callback migrations (`CORTEX_MIGRATION` / `DbMigration`)** — versioned `up` functions registered in code. Still supported unchanged; versions are stored in the same **`schema_migrations`** table using **14-digit zero-padded** numeric strings that sort **before** timestamp versions (below sentinel **`19900101000000`**).

**Precedence and coexistence:** On each `db:migrate` run, **C callbacks run first**, then **SQL file migrations** (lexicographic order by version string). Both record rows in **`schema_migrations`**. Legacy databases with **`schema_migrations(version INTEGER)`** or the old **`cortex_sql_migrations`** table are **upgraded automatically** on first access. JSON storage (path ending in **`.json`**) remains for tests only.

**API highlights:**

- **`db_migrate()`** / **`db_migrate_default()`** — SQLite file path or legacy JSON; opens a connection, runs C registry + SQL files, then writes **`db/schema.sql`** on SQLite only.
- **`db_migrate_on_connection()`** / **`db_migrate_default_on_connection()`** — Same pipeline on an existing **`DbConnection`** (used for PostgreSQL bootstrap and tooling).
- **`db_migration_migrate_sql_files()`**, **`db_migration_rollback_sql_files()`**, **`db_migration_status_sql_files()`**, **`db_migration_generate()`** — see **`db/db_migration_runner.h`**, **`db/db_migration_parser.h`**, **`db/db_migration_schema.h`**.

**`schema_migrations` table (unified):** `version` (TEXT, 14-char timestamp or padded C version), `name`, `applied_at` (`SQL_DATETIME` / `SQL_NOW` via backend-appropriate DDL in **`db_migration_schema.c`**).

**C generator note:** **`db/db_migration_generator.c`** still generates **C stubs** under **`db/migrations/`** for historical workflows. Prefer **`cortex generate migration <name>`** for new **`.sql`** files in **`db/migrate/`**.

### CLI (database)

| Command | Behavior |
|---------|----------|
| `cortex db:create` | Creates **`db/`** and the default SQLite file for the current **`CORE_ENV`**. |
| `cortex db:migrate` | Runs pending **C + SQL** migrations (SQLite path, or PostgreSQL when **`DATABASE_URL`** / **`DB_ADAPTER=postgresql`** is set and the binary was built with libpq). |
| `cortex db:rollback [N]` | Rolls back the last **N** SQL file migrations (default **1**). Uses each file’s **`-- migrate:down`** section inside a transaction. |
| `cortex db:status` | Lists migration files and **up** / **down** state. |
| `cortex generate migration <name>` | Creates a timestamped **`.sql`** stub under **`db/migrate/`** (snake_case **name**). |

Alternate forms: `cortex db create`, `cortex db migrate`, `cortex db rollback [N]`, `cortex db status`.

### New projects (`cortex new`)

- Creates **`db/`** and **`db/development.sqlite3`** immediately (empty database file).
- Generated **`main.c`** includes **`db/db_bootstrap.h`** and calls **`cortex_db_bootstrap()`** before serving, and **`cortex_db_shutdown()`** when the server loop ends.
- **Development / test (`CORE_ENV` not `production`):** **`cortex_db_bootstrap()`** runs **`db_migrate_default()`** (SQLite) or **`db_migrate_default_on_connection()`** (PostgreSQL) so the schema is brought up automatically; failure **aborts** startup.
- **Production:** pending migrations **do not** run automatically; **`LOG_WARN`** reminds you to run **`cortex db:migrate`**, then the pool initializes.

### Building the framework and SQLite

- `libcortex.a` embeds the official **[SQLite amalgamation](https://www.sqlite.org/amalgamation.html)** (`sqlite3.c`) so **generated apps only link** `-lcortex -lm` for SQLite-only builds (no separate `-lsqlite3` on the app). When the Makefile enables PostgreSQL, **`libpq` is also linked** (`-lpq` via `pkg-config`). **Apps that link a `libcortex.a` built with PostgreSQL support** (for example `sqapp` or custom binaries) must pass the **same `-lpq`** on the link line or they will see unresolved `PQ*` symbols.
- On the **first** `make` in the Cortex repo, `vendor/sqlite/sqlite3.c` (and headers) are fetched by **`scripts/fetch-sqlite-amalgamation.sh`** (requires network once). The script is idempotent; large files are listed in `.gitignore` so clones stay small.
- Offline: run the script after copying the amalgamation into `vendor/sqlite/`, or unpack the official zip there.

### HTTP port (for local servers and tests)

- The embedded HTTP server listens on **`CORE_PORT`**, default **3000** (`core_config_load()`). Tests may set `CORE_PORT` to avoid clashes with other services on port 3000.

## Logging and Observability (Pulse)

Cortex ships with **Pulse**, a native, dependency-free structured logging
subsystem. It delivers a Rails-like developer experience in TEXT mode and
emits valid **NDJSON** (one JSON object per line) for production aggregators
— no third-party libraries, no heap allocations on the hot path, thread-safe
via `pthread` mutex.

The public API lives in **`core/pulse.h`** / **`core/pulse.c`**.

### Bootstrap

Executables built on `libcortex` initialize Pulse automatically through
`cortex_main_bootstrap()` (see `core/main.c`), which the `cortex` CLI invokes
on startup and pairs with `cortex_main_shutdown()` on exit. Inside library
code or tests you can call the public API directly:

```c
#include "core/pulse.h"

PulseConfig cfg = {0};
cfg.level    = PULSE_INFO;
cfg.format   = PULSE_FMT_TEXT;
cfg.output   = stderr;
cfg.colorize = 1;
pulse_init(&cfg);

pulse_log(PULSE_INFO, "boot", "service ready on port %d", 3000);

pulse_log_fields(PULSE_INFO, "action", "request completed",
                 "method", "GET",
                 "path",   "/posts",
                 "status", "200",
                 NULL);

pulse_shutdown();
```

Convenience macros (`LOG_DEBUG`, `LOG_INFO`, `LOG_WARN`, `LOG_ERROR`,
`LOG_FATAL`, `LOG_WITH`) wrap `pulse_log` / `pulse_log_fields` for the common
case. Calling any logging function before `pulse_init` is a silent no-op so
libraries can instrument themselves without forcing a logger lifecycle on
their consumers — except `PULSE_FATAL`, which always `abort()`s.

### Levels

| Level          | Use case                                  |
|----------------|-------------------------------------------|
| `PULSE_DEBUG`  | Verbose diagnostics (e.g. fast SQL).      |
| `PULSE_INFO`   | Normal lifecycle and request events.      |
| `PULSE_WARN`   | Recoverable anomalies (e.g. slow SQL).    |
| `PULSE_ERROR`  | Errors that did not stop the process.     |
| `PULSE_FATAL`  | Unrecoverable — calls `abort()`.          |
| `PULSE_SILENT` | Suppress every emit (FATAL still aborts). |

### Environment-driven configuration

`pulse_init_from_env()` (called by `cortex_main_bootstrap()`) reads:

| Variable          | Behavior                                                                                                |
|-------------------|---------------------------------------------------------------------------------------------------------|
| `CORE_ENV`        | Selects an environment profile (see below). Defaults to `development`.                                  |
| `CORE_LOG_LEVEL`  | Override level: `debug`, `info`, `warn`, `error`, `fatal`, `silent`.                                     |
| `CORE_LOG_FILE`   | If set, Pulse opens that file for append, owns it, and rotates it (10 MiB × 5 files by default).         |

| `CORE_ENV`     | Format | Output  | Default level | Colors |
|----------------|--------|---------|---------------|--------|
| `development`  | TEXT   | stderr  | `INFO`        | auto (TTY) |
| `test`         | TEXT   | stderr  | `WARN`        | off    |
| `production`   | JSON   | stdout  | `INFO`        | off    |

ANSI colors are always disabled when writing to a file, regardless of the
`colorize` config flag.

### Output formats

Development (TEXT, colorized when on a TTY):

```text
2026-05-09 14:32:01.042  INFO   [action]  request completed  method=GET  path=/posts  status=200  duration=4ms
```

Production (NDJSON, one object per line):

```json
{"ts":"2026-05-09T14:32:01.042Z","level":"INFO","module":"action","msg":"request completed","method":"GET","path":"/posts","status":"200","duration":"4ms"}
```

JSON values are escaped per ECMA-404 (quotes, backslashes, control
characters as `\uXXXX`). Timestamps are local in TEXT mode and UTC in JSON
mode, both with millisecond precision.

### File rotation

When `cfg.log_file` (or `CORE_LOG_FILE`) is set, Pulse rotates on size:

```text
app.log         # current (active stream)
app.log.1       # most recent backup
app.log.2
...
app.log.N       # oldest, dropped when N == max_files
```

`stdout` and `stderr` are never rotated.

### Automatic framework integrations

Once Pulse is initialized, several Cortex modules emit structured events
without any extra wiring:

- **`action/action_dispatch.c`** — every dispatch logs a Rails-like
  `request completed` event with `method`, `path`, `status`, `duration`.
  HTTP status family maps to level: 2xx/3xx → `INFO`, 4xx → `WARN`,
  5xx → `ERROR`.
- **`db/sqlite/sqlite_adapter.c`** — every `db_connection_exec` is timed
  with `CLOCK_MONOTONIC`. Successful fast queries log at `DEBUG`, queries
  slower than 100 ms log at `WARN` (`slow query`), and failures log at
  `ERROR` with `sql`, `duration`, `rc`, and `error` fields. Prepare/step
  failures also surface as `ERROR` events.
- **`core/cortex_error.c`** — `cortex_err_print()` no longer writes to
  stderr directly; every `CortexError` is converted into a structured Pulse
  event under the `cortex.error` module with `code`, `source`, `at`,
  `errno` fields.
- **`core/main.c`** + **`cli/cortex_main.c`** — Pulse is initialized from
  the environment at process start and shut down on exit, with
  `framework starting` / `framework shutting down` events on either side.

## Getting Started

1. Build Cortex:

   ```bash
   make
   ```

2. Create a new Cortex project:

   ```bash
   ./cortex new blog
   ```

3. Enter the project directory, run migrations, and start the web server:

   ```bash
   cd blog
   ../cortex db:migrate
   make server
   ```

   `make server` now builds JavaScript assets automatically before boot.

   (Alternatively, if you run from inside the project directory, `./cortex
   server` will detect “project mode” and delegate to the project `Makefile`.)

4. Open `http://localhost:3000`. You should see the Cortex welcome page.

5. Database (required before boot when there are pending migrations): if startup reports pending migrations, run:

   ```bash
   ../cortex db:create
   ../cortex db:migrate
   ```

6. Generate scaffold endpoints:

   ```bash
   ../cortex generate scaffold Post title:string body:text
   ```

   Then restart the server (`make server`) so the new controllers and routes
   are compiled and linked.

## JavaScript layer (Rails-like + React islands)

Cortex now includes a native JavaScript layer designed for server-rendered apps,
following a Rails-like "convention over configuration" workflow.

### What is included

- React islands mounted automatically from server-rendered templates
- Scaffold-driven React pages/components for each resource
- JSON endpoints (`.json`) generated with the scaffold for frontend data
- `esbuild` as the default bundler
- Asset fingerprinting + manifest support for production
- Automatic script injection in rendered HTML (no manual `<script>` tag needed)

### Directory conventions

Generated projects and scaffold workflows use:

```text
app/javascript/
  application.jsx
  resources/
    index.jsx
    <resource_plural>/
      index.jsx
  # Optional (legacy/non-React scaffold path):
  application.js
  controllers/
    index.js
    *_controller.js
js/runtime/
  index.js
public/assets/
  manifest.json
  application-<hash>.js
```

React mount convention:

- HTML usage: `data-react-component="postsIndexPage"`
- props payload: `data-react-props='{"indexJsonPath":"/posts.json"}'`
- component registry: `app/javascript/resources/index.jsx`

### CLI commands

Generate common MVC building blocks:

```bash
./cortex generate controller users
./cortex generate resource posts
./cortex generate model post
./cortex generate service mailer
```

Generate a Stimulus-style controller:

```bash
./cortex generate stimulus post
```

Generate scaffold with React islands (default):

```bash
./cortex generate scaffold Post title:string body:text
```

Disable React for scaffold generation:

```bash
./cortex generate scaffold Post title:string body:text --no-react
```

Generate Neural module starters:

```bash
./cortex generate neural:model incident
./cortex generate neural:prompt incident
./cortex generate neural:agent incident
./cortex generate neural:rag incident
./cortex generate neural:stream incident
./cortex generate neural:memory incident
./cortex generate neural:retriever incident
./cortex generate neural:integration openai
./cortex generate neural:policy incident
```

Neural generator outputs:

- `neural:model` -> `app/neural/<name>_neural_model.c` with `system_prompt` and enrich hook.
- `neural:prompt` -> `app/neural/prompts/<name>_prompt.c` with `neural_prompt_render` starter template.
- `neural:agent` -> `app/neural/agents/<name>_agent.c` with tool registration/bootstrap flow.
- `neural:rag` -> `app/neural/rag/<name>_rag.c` with `RAGEngine` init/store/query helpers.
- `neural:stream` -> `app/neural/streams/<name>_stream.c` with token callback streaming stub.
- `neural:memory` -> `app/neural/memory/<name>_memory.c` with remember/recall wrappers.
- `neural:retriever` -> `app/neural/retrievers/<name>_retriever.c` with embed/index/search wrappers.
- `neural:integration` -> `app/neural/integrations/<name>_integration.c` with `LLMIntegration` adapter starter.
- `neural:policy` -> `app/neural/policies/<name>_policy.c` with prompt guardrails and token budget defaults.

Build JavaScript assets for production:

```bash
./cortex assets:build
```

Run development mode (JS watch + server):

```bash
./cortex dev
```

`cortex new` now generates a minimal `package.json` with `react`, `react-dom`,
and `esbuild`. `cortex assets:build` and `cortex dev` automatically run
`npm install` when dependencies are missing, so no manual frontend setup is
required.

### Scaffold integration

`generate scaffold` now integrates React islands automatically.

Example:

```bash
./cortex generate scaffold Post title:string body:text
```

In addition to model/controller/views/routes, Cortex also generates:

- `app/javascript/resources/posts/index.jsx`
- `app/javascript/resources/index.jsx`
- `app/neural/post_neural_model.c` (AI integration starter template)
- React mount points in scaffolded HTML:
  - `data-react-component`
  - `data-react-props`
- Rails-like JSON endpoints for each resource:
  - `GET /posts.json`
  - `GET /posts/:id.json`

### Layout/render integration

When HTML is rendered through `Action View`, Cortex injects the JavaScript bundle
automatically. In production mode, it resolves the hashed bundle name via
`public/assets/manifest.json`.

## Contributing

Contributions are welcome.

Please open an issue for bugs or feature requests, and submit pull requests
with your improvements.

### License
Cortex is released under the [MIT License](LICENSE).
Copyright (c) 2026 Sergio Maia
