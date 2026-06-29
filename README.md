# Kungfu

Kungfu is a framework and runtime for building journal-first,
streaming-data applications.

At its core is a low-latency, append-only event journal with a shared,
strongly-typed schema, exposed zero-copy to C++, Python, and Node. Kungfu
offers these capabilities — the journal, in-process state, and deterministic
replay — as a foundation (SDK) to build on, and ships a minimal reference
application built on that foundation.

Originally created for trading execution, the core is general: anything that
needs to capture, share, and faithfully replay high-frequency event streams.

## Core ideas

- **Journal-first data plane** — one append-only event log
  ([`yijinjing`](framework/core)) with a unified type system
  ([`longfist`](framework/core)) carrying source / destination / nanosecond
  timestamp / message type. Every component consumes the same frames rather than
  inventing its own format.
- **Zero-copy, multi-language runtime** — the same in-process journal data is
  shared across C++, Python, and Node (N-API) without serialization on the hot
  path.
- **Deterministic replay** — live and replay run on the same runtime and the
  same journal semantics, so recorded streams replay with high precision rather
  than through a separate engine.

## Build on it

Kungfu is meant to be a base others build on, not a single fixed application:

- **Capabilities SDK** — consume journal / state / replay from your own code in
  C++, Python, or Node.
- **Extension points** — add features and UI to a Kungfu application.
- **Application SDK** — assemble a complete application on top of Kungfu, with
  Kungfu as the underlying dependency.

The repository ships the core plus a minimal reference application that
demonstrates and exercises these capabilities.

## Components

- **Core (C++)** — `longfist` (type system) and `yijinjing` (journal runtime),
  with Python and Node bindings, packaged as `@kungfu-trader/kungfu-core`.
- **`kfc`** — the runtime command-line interface (`kfc --version`, journal
  subcommands, …).
- **Reference app** — a cross-platform desktop application built on Electron
  with a React / TypeScript frontend.

Runs on Windows, macOS, and Linux (including arm64).

## Getting started

See [`CONTRIBUTING.md`](CONTRIBUTING.md) for the toolchain, build steps, coding
conventions, and the pull request / release flow.

```sh
# install fnm once, then:
git clone git@github.com:kungfu-systems/kungfu.git
cd kungfu
./kungfu-code sync && ./kungfu-code build
```

## Documentation

- [`CONTRIBUTING.md`](CONTRIBUTING.md) — toolchain, build, conventions, releases.
- [`docs/version-release-design.md`](docs/version-release-design.md) — versioning
  and release mechanism rationale.
- [`framework/core/docs/adr/`](framework/core/docs/adr) — architecture decision
  records.

## License

[Apache License 2.0](LICENSE).
