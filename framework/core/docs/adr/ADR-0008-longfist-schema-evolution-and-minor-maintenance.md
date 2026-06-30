# ADR-0008: the longfist binary layout is the true cross-time / cross-language compatibility invariant; schema-evolution and minor-maintenance policy

- Status: proposed (establishes the policy; governance implementation pending. longfist = FlatBuffers and the zero-copy journal are already implemented on this line)
- Date: 2026-06-29
- Category: (b) mechanism / governance — data-format version governance
- Subsystem: longfist type system + yijinjing journal (in-process zero-copy across C++/Python/Node + mmap cross-process + on-disk replayable)
- Related: paired with the version mechanism `docs/version-release-design.md` (this ADR is its "true invariant" layer); takes over from ADR-0002 (longfist hana→FlatBuffers, a one-time migration) into the standing "evolution policy" governance; independent of ADR-0001 (journal publish barrier)

## Decision

Establish the **longfist binary layout** as kungfu's **true compatibility
invariant**, and from it define the schema-evolution and minor-maintenance
policy:

1. **The true invariant lives beneath the tag.** The git tag, the floating
   channel ref, and the `package.json` version are all downstream projections of
   "which longfist layout this artifact speaks." The thing that actually bears
   weight — the thing that actually makes a consumer unable to read the data — is
   the longfist layout itself. The version mechanism exists to protect this
   invariant.

2. **Layout is ABI.** Zero-copy means the in-memory layout == the journal/wire
   layout, and reads do not go through parsing. So a consumer compiled against
   one layout **cannot read another layout by comparing version numbers or by
   feature detection** — it can only speak the same layout, or do an
   evolution-aware explicit decode on a **non-zero-copy path**.

3. **Hot path pinned per-minor, cold path tolerates evolution (a hybrid).**
   - **Hot path** (in-process zero-copy, real time): the layout is frozen within
     a minor, with no runtime translation across minors. This forces the shape
     "each minor is a layout epoch, maintained in parallel" (see the "Minor lines
     are long-lived" section of `docs/version-release-design.md`).
   - **Cold path** (journal on-disk replay / load): must guarantee "a journal
     written years ago is still replayable today." Here, **FlatBuffers' additive
     schema evolution** (add-only fields, optional, default, no renumbering, no
     changing existing field semantics) lets a **new reader decode an old
     layout**, bounding the cost of the hot path's parallel maintenance to an
     acceptable range.

4. **The compatibility promise is counted per minor, expressed as "load only if
   runtime ≥ schema."** An extension / consumer built against a minimum longfist
   schema version should load on any runtime ≥ that version; breaking layout
   changes happen only at a major and are explicitly announced. The maturity of
   the promise must be stated honestly, without pretending to a stability that
   has not been built.

## Background and problem

kungfu distributes prebuilt binaries + a capability SDK consumed by external
products (ADR-0006). Its compatibility problem is the same class as a Python C
extension / Node native addon (consumed at compile / ABI time, with runtime
feature detection physically inapplicable), but with **one extra dimension that
neither Python nor Node has**: longfist is both zero-copy across languages and
**persisted into the journal to be replayed**, so compatibility carries a **time
dimension** (old data must remain readable).

Historically the docs governed only the release line (channel / tag /
version-state, see `docs/version-release-design.md`) and treated longfist as
"schema / disk format" in passing (ADR-0001's header note "does not touch the
schema / disk format" is exactly this). But no document **named the longfist
layout as the true invariant**, and there was no written schema-evolution or
minor-compatibility policy — **the tool (FlatBuffers) was chosen correctly, but
the governance policy was empty**. This ADR fills that gap.

## Comparison

For compatibility governance of "a compiled-extension ecosystem + a persisted
data format," the industry's converged answer is "stable subset + runtime ≥ API":

- **Python**: the full ABI is locked per minor (`cp3x`), with a separate Stable
  ABI / `abi3` (compiled against a minimum version, loadable on a newer runtime).
- **Node**: Node-API is a stability layer that decouples the addon from V8 ABI
  churn; `NAPI_VERSION` is exactly runtime ≥ API.
- **Data-format side**: the additive schema evolution of FlatBuffers / Protobuf /
  Avro; the long-term on-disk compatibility of SQLite / Postgres (solving "data
  written years ago is still readable").

longfist's problem is those two **stacked and harder** (function ABI +
cross-language zero-copy data layout + persistence time dimension), so the policy
takes their intersection: stable subset (FlatBuffers additive evolution) +
runtime ≥ schema + cold-path cross-version decode.

## Current state (implemented vs to-be-built)

- **Implemented** (fact on this line): longfist uses FlatBuffers (`framework/core`'s
  `*.fbs` + `flatc` codegen); in-process zero-copy + mmap cross-process + journal
  on-disk; ADR-0002's hana→FB migration.
- **To be built** (established by this ADR, implementation pending): written
  schema-evolution rule checks (CI to block breaking changes such as "modify an
  existing field / renumber"); the "runtime ≥ schema" load gate; per-minor
  compatibility-window declarations; a cold-path replay cross-version decode test
  baseline.

## Violation / replacement criteria

Any change to the longfist layout or its distribution that **fails to preserve
any one of the following is a downgrade** and must be recorded in this ADR:

1. an old journal can still be replayed by a new binary on the cold path (or an
   explicit migration is provided); historical replayability must not be silently
   broken;
2. the longfist layout is frozen within a minor, and breaking changes happen only
   at a major and are explicitly announced;
3. the hot path introduces no runtime translation for compatibility (otherwise
   the zero-copy moat fails);
4. every minor line that still has users stays "maintainable by anyone" (no
   foreclosure; see the five maintainability-openness points of
   `docs/version-release-design.md`).
