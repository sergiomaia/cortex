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

In Cortex, views are plain HTML templates located under `app/views/...` and
rendered by `Action View` helpers.

For example, generated controllers call `render_view(res, "home/index")`,
which maps to a template file like `app/views/home/index.html`.

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
- **DB**: SQLite-backed persistence (via `db_connection` + `db/sqlite/sqlite_adapter`)

## Database (SQLite)

Cortex uses **SQLite as the default database**, in the spirit of Rails: **zero YAML**, **convention over configuration**, and a project that runs after `cortex new` without manual DB setup.

### Convention (paths and environment)

| `CORE_ENV`    | Database file              |
|---------------|----------------------------|
| `development` | `db/development.sqlite3`   |
| `test`        | `db/test.sqlite3`         |
| `production`  | `db/production.sqlite3`    |

- `CORE_ENV` defaults to **`development`** when unset (same as `core_config_load()`).
- The path helper is `db_path_for_environment()` in `db/db_paths.c` / `db/db_paths.h`.

### Architecture

- **`db/db_connection.h`** — Small, engine-agnostic API: connect, `exec`, prepared statements (`prepare` / `step` / `bind_int` / `column_int` / `finalize`), close. Call sites should depend on this header, not on SQLite directly.
- **`db/sqlite/sqlite_adapter.c`** — Implements that API with the SQLite C API (`sqlite3.h`).
- **`db/db_bootstrap.h`** — Application lifecycle:
  - **`cortex_db_bootstrap()`** — Checks pending migrations before boot; if any are pending, it prints guidance to run **`cortex db:migrate`** and aborts startup. When up to date, it opens a **single** process-wide connection via **`cortex_db_init()`** (reuse across requests in the same process).
  - **`cortex_db_shutdown()`** — Closes the connection on exit.
  - **`cortex_db_exec(const char *sql)`** — Convenience wrapper around the active connection (for migrations or app code that runs SQL strings).

### Migrations

- **Default (SQLite):** executed versions are stored in the **`schema_migrations`** table (`version INTEGER PRIMARY KEY`).
- **Legacy:** if the migration “storage” path ends with **`.json`**, the older JSON array format is still supported (mainly for tests); new apps should use SQLite paths only.
- **`db_migrate()`** / **`db_migrate_default()`** — Register C `up` callbacks (`DbMigration` + `active_migration_*`). Passing **`NULL`** as the path selects the default SQLite file for the current environment.
- **`db/db_migration_generator.c`** — Can generate stub migration files under `db/migrations/` (convention for future SQL-based workflows).

### CLI

| Command            | Behavior |
|--------------------|----------|
| `cortex db:create` | Creates **`db/`** and the default SQLite file for the current **`CORE_ENV`** (no path argument). |
| `cortex db:migrate` | Runs pending migrations against that same default database. |

Alternate forms: `cortex db create`, `cortex db migrate`.

### New projects (`cortex new`)

- Creates **`db/`** and **`db/development.sqlite3`** immediately (empty database file).
- Generated **`main.c`** includes **`db/db_bootstrap.h`** and calls **`cortex_db_bootstrap()`** before serving, and **`cortex_db_shutdown()`** when the server loop ends. Boot now fails fast when migrations are pending, matching the Rails-style warning flow.

### Building the framework and SQLite

- `libcortex.a` embeds the official **[SQLite amalgamation](https://www.sqlite.org/amalgamation.html)** (`sqlite3.c`) so **generated apps only link** `-lcortex -lm` (no separate `-lsqlite3` on the app).
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

# ## License
# Cortex is released under the [MIT License](LICENSE).
# Copyright (c) 2026 Sergio Maia
