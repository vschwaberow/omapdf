# AGENTS.md

Operating contract for coding agents. `README.md` is for humans.

## Precedence

Highest wins. If two rules conflict, obey the higher one and say so.

1. The current user message
2. `AGENTS.local.md` (gitignored, optional, machine-specific)
3. The nearest `AGENTS.md` to the file you edit
4. This file
5. The matching skill, after you read it
6. Training defaults

## Hard stops

- Do not run `git push` or any remote publish.
- Do not mention AI, tools, models, or co-authors in commits, pull requests, bodies, or docs.
- Do not leave a `Co-authored-by` trailer or a `Made with Cursor` line in commits or PR bodies. Install `hooks/commit-msg` into `.git/hooks` or `core.hooksPath`. After any `gh pr create`/`edit`, strip the footer from the PR body before you stop.
- Do not commit agent state: `.cursor/`, `.claude/`, `.codex/`, `.crush/`, `.aider*`, session logs, scratchpads.
- Do not commit `AGENTS.local.md` or `AGENTS.md.local`.
- Do not add comments, docstrings, or tutorial text in owned source. Names and structure carry the meaning.
- Do not create branches, worktrees, or remotes unless the user asks.
- Do not create an unsigned commit. If signing fails, retry once. If it fails again, do not commit. Tell the user.
- Do not skip hooks.
- Do not amend a commit you did not create in this session.

## Code

Obey `karpathy-guidelines` on every code change. Read the skill first.

Write clean code: small functions, names that state the meaning, no dead paths, no magic values.

Deduplicate. One fact, one place. Search before you write. If the same block appears a third time, extract it.

## Knowledge

If `AGENTS.local.md` exists, read it before non-trivial work.

Otherwise use `README.md`, this file, and the tree.

Read the skill that matches the task before you act. Do not reprint skills here.

## Layout

```
src/
  main.cpp                 entry; QML engine; context properties
  app/                     static library omapdf_core
    AppController          windows/tabs, open paths, recents, print
    TabModel                 tab list model (path, title, id)
    SessionStore             per-file view state on disk
    ThemeBridge              colors from Omarchy TOML
    AnnotStore               sidecar highlight/note storage
    StructureEngine          PDF page ops via libqpdf (async)
    LinkGuard                external link confirmation
    LocaleTranslator         qsTr fallback from JSON
    DocumentLimits           max pages / file size guards
qml/
  Main.qml                   ApplicationWindow, shortcuts, SplitView
  chrome/                    TabStrip, SearchBar, ToolRail, dialogs, buttons
  viewer/
    PdfPane.qml              one tab: document + view + annots
    OmapdfMultiPageView.qml  Qt PdfMultiPageView fork (scroll, select, search)
    ThumbnailPane.qml        optional thumbnail rail
    OutlinePane.qml          bookmark tree
    WelcomePage.qml          empty state
tests/                       Qt Test targets registered in CMakeLists.txt
docs/usage.md                shortcuts and optional power features
translations/                DE strings (i18n.qrc)
hooks/commit-msg             strip attribution trailers from commit messages
scripts/                     install helpers, clang-tidy wrapper
assets/                      logo and desktop icon sources
```

QML context properties from `main.cpp`: `app`, `theme`, `structure`, `omapdfMaxPageCount`.

Runtime state directory: `~/.local/state/omapdf/` (session JSON, annot sidecars).

## Commands

- Configure release: `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release`
- Build: `cmake --build build`
- Run: `./build/omapdf [files...]`
- Debug: `cmake -B build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build-debug`
- Tests: `ctest --test-dir build-debug --output-on-failure`
- Clang-tidy: `cmake -B build-tidy -G Ninja -DCMAKE_BUILD_TYPE=Debug -DOMAPDF_CLANG_TIDY=ON && cmake --build build-tidy`

CTest targets: `theme_bridge`, `annot_store`, `structure_engine`, `flatten_export`, `locale_translator`, `reading_gate`, `app_controller`, `session_store`.

## Constraints

- C++26, Qt 6.8+ (`Core`, `Gui`, `Quick`, `QuickControls2`, `Pdf`, `Widgets`, `PrintSupport`, `Concurrent`, `Qml`).
- Structure ops use `libqpdf`. Do not link Poppler (GPL).
- `OmapdfMultiPageView.qml` is a vendor fork. Keep upstream doc blocks; limit diffs to owned behavior.
- After a libqpdf write, reload the Qt PDF document. One writer per open file per session.

## Git

- Work on the current branch.
- Stage named files only. Do not run `git add .`.
- One change: one short subject. Several changes: subject plus `-` dash list.

## Done when

- Build succeeds and affected `ctest` targets pass.
- The change is signed on the current branch, or signing failed and no commit was made.
- Nothing was pushed.

Write only what the agent cannot infer from the tree. Root file under 150 lines.
