## cortex

Minimal C99 skeleton for an AI‑native, Rails‑inspired web framework.

This scaffold is intentionally simple and focused on structure so each layer
can be implemented incrementally:

- **Core**: runtime, config, logging, container
- **Action**: HTTP routing, controllers, requests, responses, views
- **Active**: data and ORM primitives
- **Neural**: AI/LLM models, prompts, memory, retrieval, agents, streaming
- **Flow**: jobs, queues, workers, pipelines
- **Forge**: project and code generators
- **Guard**: authentication, policies, and security helpers
- **Pulse**: logging, metrics, tracing, AI observability
- **Cache**: in‑memory and external cache abstractions
- **CLI**: `cortex` command entrypoint
- **App**: example application code (controllers, models, neural)

Build with:

```bash
make
```

Run tests with:

```bash
make test
```

