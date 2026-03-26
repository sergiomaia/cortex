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
- **Pulse**: logging, metrics, tracing, AI observability
- **Cache**: in-memory and external cache abstractions
- **CLI**: the `cortex` command entrypoint
- **App**: example application code (controllers, models, neural)

## Getting Started

1. Build Cortex:

   ```bash
   make
   ```

2. Create a new Cortex project:

   ```bash
   ./cortex new blog
   ```

3. Enter the project directory and start the web server:

   ```bash
   cd blog
   make server
   ```

   (Alternatively, if you run from inside the project directory, `./cortex
   server` will detect “project mode” and delegate to the project `Makefile`.)

4. Open `http://localhost:3000`. You should see the Cortex welcome page.

5. Create storage and apply migrations (inside `blog/`):

   ```bash
   ../../cortex db:create
   ../../cortex db:migrate
   ```

6. Generate scaffold endpoints:

   ```bash
   ../../cortex generate scaffold Post title:string body:text
   ```

   Then restart the server (`make server`) so the new controllers and routes
   are compiled and linked.

## JavaScript layer (Rails-like + Stimulus-style)

Cortex now includes a native JavaScript layer designed for server-rendered apps,
following a Rails-like "convention over configuration" workflow.

### What is included

- A lightweight Stimulus-style runtime (`Controller` + `Application`)
- Automatic controller discovery/registration by naming convention
- `esbuild` as the default bundler
- Asset fingerprinting + manifest support for production
- Automatic script injection in rendered HTML (no manual `<script>` tag needed)

### Directory conventions

Generated projects and scaffold workflows use:

```text
app/javascript/
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

Controller naming rule:

- file: `app/javascript/controllers/post_controller.js`
- identifier: `post`
- HTML usage: `data-controller="post"`

### CLI commands

Generate a Stimulus-style controller:

```bash
./cortex generate stimulus post
```

Build JavaScript assets for production:

```bash
./cortex assets:build
```

Run development mode (JS watch + server):

```bash
./cortex dev
```

### Scaffold integration

`generate scaffold` now integrates JavaScript behavior automatically.

Example:

```bash
./cortex generate scaffold Post title:string body:text
```

In addition to model/controller/views/routes, Cortex also generates:

- `app/javascript/controllers/post_controller.js`
- Stimulus-style attributes in scaffolded HTML:
  - `data-controller`
  - `data-action` (for example `submit->post#submit`)
  - `data-post-target` (target bindings)

### Layout/render integration

When HTML is rendered through `Action View`, Cortex injects the JavaScript bundle
automatically. In production mode, it resolves the hashed bundle name via
`public/assets/manifest.json`.

## Contributing

Contributions are welcome.

Please open an issue for bugs or feature requests, and submit pull requests
with your improvements.

## License

License terms depend on the upstream project license.
If a `LICENSE` file is not present in your checkout, please refer to the
repository metadata for the authoritative license information.
