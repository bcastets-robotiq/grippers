# API documentation

How the generated API reference works, the conventions the header
comments follow, and how to build and preview it locally — at
increasing fidelity — before sending a doc change for review.

## How it fits together

Two separate pipelines, in two separate repos:

1. **This repo** owns the doc comments in `include/Robotiq/**/*.hpp`,
   [`Doxyfile`](Doxyfile), and [`.doxybook/config.json`](.doxybook/config.json).
   Running [Doxygen](https://www.doxygen.nl/) against the headers
   produces both `doxygen-xml/` (consumed by Doxybook2, below) and
   `doxygen-html/` (a standalone local preview) — both gitignored.
2. **[robotiq.github.io](https://github.com/robotiq/robotiq.github.io)**
   (the docs site) checks this repo out as a submodule, runs
   [Doxybook2](https://github.com/matusnovak/doxybook2) over the XML to
   render it as Markdown, and publishes it as the SDK's API reference
   page. See that repo's `contribute.mdx` for the site side of the
   pipeline — this document only covers the part that lives here.

This repo's job is to produce clean, warning-free XML — the HTML
preview and the local Doxybook2 run described below are for *your own*
review before the docs site ever sees the change; neither is generated
in CI or committed.

## Conventions the headers follow

A few rules the existing headers all follow, worth keeping consistent
when you add or edit documentation. The first four are about Doxygen
itself; the last three are lessons learned by actually running the
output through Doxybook2 — they render fine in Doxygen's own HTML but
break in the Markdown the site actually uses, so they're easy to miss
unless you check both (see [Previewing the rendered
output](#previewing-the-rendered-output)):

- **Use `//!`, never plain `//`, for anything meant to be
  documentation.** Doxygen only recognizes `//!` and `///` as special
  comments — a plain `// like this` is invisible to it, no matter how
  documentation-shaped it reads.
- **Put the doc comment directly above the thing it documents** — the
  `class`/`struct`/`enum`/function declaration itself, not floating
  above `#pragma once` and the `#include`s. A comment block that isn't
  immediately in front of a declaration gets filed as that *file's*
  documentation instead, silently, with no warning.
- **Tag the pieces, don't just prose them**: `\param`, `\return`,
  `\throw`, `\tparam` for template parameters, `\warning` for anything
  that moves the fingers or otherwise needs a callout, and
  `\code{.cpp} ... \endcode` for a short worked example on the types
  people will actually reach for first (see `Gripper`, `GripperCommand`,
  `Serial` for examples).
- **`\name Section title` / `\{ ... \} `** groups related members
  (e.g. constants) under a labeled subsection within one class or
  namespace page — see `register_map.hpp`. Don't use the bare `\{ \}`
  form without `\name`: it silently attaches its heading text as the
  *first member's own* brief instead of a section label, which makes
  that one member's documentation deeper than its neighbors for no
  reason.
- **Use `\code`, never `\verbatim`, for a plain preformatted block**
  (e.g. an ASCII diagram or table). Both look identical in Doxygen's
  own HTML, but Doxybook2 doesn't strip the `//!` comment-continuation
  marker from `\verbatim` content the way it does everywhere else —
  every line renders with a literal `//!` prefix. `\code` (with or
  without a `{.cpp}` language tag) doesn't have this problem.
- **Use `\par Example`, never a raw Markdown heading (`### Example`),
  inside a doc comment.** Doxygen's own HTML renders `###` as a normal
  heading, but Doxybook2 renders it as three broken, empty header
  lines. `\par <title>` is the safe equivalent — with one caveat: it's
  a distinct "section," not inline prose, so Doxybook2 (like Doxygen
  itself) pulls it out and renders it right after the brief, ahead of
  any later prose paragraphs in the same comment, rather than exactly
  where you wrote it. Usually fine for a trailing example (see
  `Gripper`'s class doc); worth checking if you rely on strict ordering.
- **Always leave a blank `//!` line before a `\code` block that follows
  a `\param`/`\return`/`\throw`.** Without it, Doxybook2 glues the code
  fence onto the same line as the preceding text and the block never
  renders as code — Doxygen's own HTML doesn't have this problem, so
  it's invisible until you check the Markdown. See `activate()` for the
  pattern that works.

## The Core API / Testing split

[`groups.dox`](include/Robotiq/groups.dox) defines the reference's
hierarchy via Doxygen's `\defgroup`/`\ingroup`, so the generated
"Topics"/"Modules" page reads as one API with `Gripper` at the center,
not a flat alphabetical dump:

- **`core_api`** — everything needed to control a real gripper, with
  `Gripper` itself listed first, and four subgroups for the types it
  uses: `commanding` (command/status blocks, faults), `connection`
  (config, connection state), `runtime` (transport/platform/logging
  extension points), `utilities` (small independent helpers).
- **`testing`** — `makeFakeGripper()` and anything else for exercising
  the SDK without hardware. Deliberately a sibling of `core_api`, not
  nested under it, so it never reads as part of the real control path.

When you add a new public type or function, add an `\ingroup <id>` line
to its doc comment so it lands in the right place instead of appearing
ungrouped.

## Building locally

Install Doxygen once — pick your platform's package manager, run from
whatever terminal you have (PowerShell, cmd, or bash all work for
these):

```
winget install --id DimitriVanHeesch.Doxygen -e   # Windows
sudo apt install doxygen                          # Debian/Ubuntu
brew install doxygen                              # macOS
```

Then, from `sdk_cpp/` — same command in any shell:

```
doxygen
```

This reads the committed `Doxyfile` and is the same command the
`api-docs` job in
[`.github/workflows/ci.yml`](../.github/workflows/ci.yml) runs on every
push and PR, and effectively what the docs site runs too. It produces
both `doxygen-xml/` and `doxygen-html/` (see below); what CI checks is
the exit code. `Doxyfile` sets `WARN_IF_UNDOCUMENTED` and
`WARN_AS_ERROR = FAIL_ON_WARNINGS`, so the command itself fails
(non-zero exit, after printing every warning it found) if anything is
genuinely undocumented, hidden behind a plain `//` comment Doxygen
can't see, or attached to the wrong declaration (see the conventions
above). CI enforces this the same way it enforces the build and
`clang-format` — a doc change that introduces a warning won't merge.

One known gap: `WARN_IF_UNDOCUMENTED` does not catch an undocumented
plain variable declared directly in the bare `namespace Robotiq` (as
opposed to a `\ingroup`-tagged one, or one in a sub-namespace like
`register_map` that itself carries a `\brief`) — Doxygen simply never
considers it for the warning in that specific shape. Not a realistic
risk given the `\ingroup` convention above, but worth knowing before
trusting a clean CI run as absolute proof of full coverage.

Clean up the generated output afterward (or just leave it — it's
gitignored):

```sh
rm -rf doxygen-xml doxygen-html                           # bash
```
```powershell
Remove-Item -Recurse -Force doxygen-xml, doxygen-html     # PowerShell
```

## Previewing the rendered output

Three levels of fidelity, cheapest first. Run:

```sh
sdk_cpp/preview_api_docs.sh      # bash — Git Bash on Windows, native on Linux/macOS
```
```powershell
sdk_cpp/preview_api_docs.ps1     # PowerShell — no Git Bash needed
```

Either script:

1. Runs `doxygen` and opens `doxygen-html/index.html` in your browser —
   **quick sanity check**. Catches most problems: missing/malformed
   comments, broken `\ref`/`\see` links, wrong grouping. It won't look
   like the final site (different theme/chrome — and per the
   conventions above, a few things render fine here but break further
   down the pipeline), but it's the fastest loop: edit, re-run, refresh.
2. If [Doxybook2](https://github.com/matusnovak/doxybook2) is on
   `PATH`, also runs it against the XML and writes `docs-api/` —
   **closer check**, the actual Markdown the docs site will receive.
   Browse it directly, or with VS Code's built-in Markdown preview
   (`Ctrl+Shift+V`). This is what catches the conventions-section
   issues above; the HTML preview alone won't.

Doxybook2 has no package-manager install — no npm, winget, or brew
package exists for it (`contribute.mdx` on the docs site currently
suggests `npm install`/`npx`, which does not work; that's a docs bug
there, not here). Download the binary for your OS from
[the latest release](https://github.com/matusnovak/doxybook2/releases/latest),
put its `bin/` folder on `PATH`, and re-run the script — it picks it up
automatically. Without it, you still get the HTML preview from step 1.

Both outputs are gitignored and disposable; the scripts only ever write
inside `sdk_cpp/doxygen-html/`, `sdk_cpp/doxygen-xml/`, and
`sdk_cpp/docs-api/` — never elsewhere, and never the committed config
files.

### Full-fidelity: previewing inside the actual docs site

Only reach for this — typically once, right before opening a PR — to
confirm the reference looks right with the site's real theme, sidebar,
and page chrome. It requires a local clone of
`robotiq.github.io` and is genuinely heavier (Node.js, `npm install`,
wiring this repo in as its `external/` submodule content). See that
repo's `contribute.mdx`, specifically "Previewing local edits to a
submodule" and "Optional: full-fidelity preview on this site" — the
steps above (1 and 2) are meant to catch everything routine first, so
you rarely need this one.

## What this doesn't cover

The per-tool wrapper page, the sidebar/table wiring, and the site's
actual theme all live in the separate `robotiq.github.io` repo and
aren't reproduced by the local Doxybook2 check above — only the
Markdown content is. A clean local run through both Doxygen and
Doxybook2 here catches everything content-related; the full-fidelity
site preview above is what's left.
