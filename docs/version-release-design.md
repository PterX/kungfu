# Version & Release Mechanism — Design Rationale

> Audience: maintainers (human or AI) evaluating, extending, or considering replacing
> this mechanism. Read this before concluding it is "legacy scaffolding".

## Why this document exists

At a glance the version/release setup looks like ordinary boilerplate: `lerna` plus a few
thin GitHub workflows that delegate to `kungfu-trader/workflows` and
`kungfu-trader/action-bump-version`. It is easy to read the code in seconds and conclude
"this is replaceable scaffolding — just swap it for changesets / semantic-release."

That conclusion is wrong, and the reason it is wrong is **not in the code** — it is in the
*orchestration intent*, which no single file reveals. This document records that intent so
the mechanism is not mistaken for replaceable boilerplate.

## Core principle: the machine fits the human

Version management here is designed around **human cognition**, not tool convenience. The
governing constraint (see the `action-bump-version` README):

> Contributors are generally not fluent with git / npm / lerna / GitHub internals. The
> pipeline must not require any of them to perform high-level tool operations. Releasing
> should be a human action that needs no scripts or commands, and version metadata must be
> maintained automatically to avoid human error.

This matters *most* precisely because the contributor pool is junior-heavy and
high-turnover — that is a reason to invest in the mechanism, not to skip it. A
declaration-based scheme would route the most judgment-heavy task (deciding the semantic
impact of a change) to the least reliable people.

## How it works (the non-obvious part)

**1. Branch channels encode version intent.** Work flows through channels
`dev → alpha → release → main` (each named `{channel}/v{major}/v{major}.{minor}`). The
*direction of a merged PR* determines the bump, automatically:

| PR merge | resulting bump |
|---|---|
| `dev → alpha` | prerelease (`x.y.z-alpha.N`) |
| `alpha → release` | patch (the formal release) |
| `release → main` | preminor (opens a new minor line + channels) |
| `main` (manual dispatch + `confirm`) | premajor |

A developer never decides "major vs minor" — the branch topology decides. **The version
judgment is hidden inside an action the developer already performs** (advancing a PR to the
next channel). No changeset files, no commit-message conventions, no manual `version`
command.

**The pipeline is asymmetric by design.** `dev` is the developer's free zone — feature
branches merge straight into `dev/v{major}/v{major}.{minor}` with no channel gate (branch
protection on `dev` requires no review). The un-cheatable trust pipeline and the
weak-centralization described below begin only at `dev → alpha`, where the first
real-binary prerelease is produced. This deliberately separates *development freedom* (fast
iteration inside dev) from *release rigor* (everything from alpha onward is gated):
**getting to the point of freezing is free; the freeze itself is strict.**

**2. The git tag is the artifact; the `package.json` version is a downstream projection.**
What carries meaning is the tag — an immutable object pinning a commit, representing "this
state is frozen and committed to." The version string inside `package.json` is merely an
npm-ecosystem projection; if it drifts it is a cosmetic mismatch, not a functional fault.

**3. A release tag carries weight = code-freeze ⊗ binary distribution, performed
atomically.** kungfu ships **prebuilt cross-platform binaries** (node-pre-gyp artifacts via
`prebuilt.libkungfu.cc`, plus the frozen `kfc`), not source for users to compile. For such
a project, a tag that does *not* guarantee the corresponding binaries are built and
distributed is an empty promise (users see `v1.0` but cannot download a `v1.0` binary). The
`alpha → release` merge therefore performs tag + full-platform build + distribution as **one
action**, so the tag has weight: *tag exists ⇒ the matching binaries exist and correspond
to it.*

> This is the key difference from source-distributed ecosystems (Go modules, Cargo /
> crates.io, Python sdist). There, "tag = frozen source" suffices because the distributed
> artifact *is* the source; they structurally never face the binary-atomicity problem.
> kungfu does, because it distributes binaries.

**4. "Worthy of being frozen" is defined by an un-cheatable pipeline, not by judgment.** A
state becomes a release only by passing the channel pipeline with machine-enforced gates
(branch protection with `isAdminEnforced=true`, not bypassable even by admins): the
three-platform `verify` (build + automated QA + code signing) is green, status checks are
strict (not stale), review is required (code-owner on the release channel), and
conversations are resolved. The mechanism deliberately makes **no judgment about whether the
code is "good"** — that is not reliably decidable, by humans or machines. It replaces an
undecidable *quality* question with a decidable *process* one: *did this state pass, without
any way to cheat, every required check?* Quality is guaranteed by the pipeline's
un-bypassability, not asserted by a person.

**5. Weak-centralization: no single point can unilaterally cut a release.** A release is the
promotion of an alpha that real users have already exercised; over the last validated alpha
the release introduces no new code (the `patch` bump only advances the version string). A
release is therefore a **consensus** produced by the pipeline (developers + real users +
three-platform build + review), not any one developer's unilateral call — which also removes
the "must not make a mistake" pressure from any individual. Developers have more say but no
unilateral authority — and that constraint deliberately includes the maintainers
themselves.

## Why not changesets / semantic-release / standard tools

All mainstream version tools share one hidden axiom: **version intent must be explicitly
declared / translated by a human** — changeset files, `feat:` / `BREAKING CHANGE:` commit
conventions, manual keyword selection. The declaration is itself a burden, an error site,
and a training requirement. These tools are optimized for **stranger collaboration**
(open-source monorepos) where you genuinely *cannot* assume a workflow, so explicit
declaration is the only option available to them.

kungfu's context is the opposite: a controlled, channelled workflow with junior /
high-turnover contributors. Here a declaration-based tool is actively *worse* — it routes
the highest-judgment task to the least reliable people. kungfu instead **reads** intent from
the branch-flow developers already perform, requiring zero extra declaration. The value
lives in the orchestration (binary ⊗ freeze atomicity, channel-encoded intent,
weak-centralization), which a *generic, packageable* tool structurally cannot provide — a
general-purpose tool must not impose one specific workflow.

(For balance: a platform *can* absorb a thin, universal opinion — the GitHub Pull Request
itself was once exactly such an addition on top of raw git. The standing bet here is that
this orchestration is thick and workflow-specific enough to remain the project's own for a
long time. GitHub "immutable releases" — GA in 2026 — is a first step toward binding a tag
to a release, but it still leaves both the *worthiness* judgment and the *atomic production*
of binaries to the user.)

## Replacement criteria (non-goals)

Before replacing this mechanism with any "standard" tool, verify the replacement preserves
the following — most standard tools silently drop them:

1. **Binary ⊗ freeze atomicity** — a tag must still guarantee the matching prebuilt
   binaries are produced and distributed, not left to a separate, driftable step.
2. **Zero developer declaration** — version intent inferred from the workflow developers
   already perform, not hand-written per change.
3. **Un-cheatable pipeline** — release-worthiness enforced by non-bypassable gates, not by
   judgment or honor.
4. **Weak-centralization** — no single actor can unilaterally cut a release; a release is
   the promotion of a user-validated prerelease.

If a candidate cannot preserve all four, it is a downgrade for this project, however
"standard" it may be.

## Pointers

- Thin workflows:
  `.github/workflows/{bump-major-version,bump-minor-version,release-new-version,release-verify}.yml`
  → reusable workflows in `kungfu-trader/workflows`.
- Bump / release / publish logic and branch-protection setup:
  `kungfu-trader/action-bump-version` (its README documents the full channel rules and the
  original design goals).
- Build & toolchain dependencies: `docs/BUILD-DEPENDENCIES.md`.
