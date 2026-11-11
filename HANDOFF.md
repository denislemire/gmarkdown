# HANDOFF — GMarkdown (2026-11-11)

Started today on the hpbsd laptop in `/home/denis/Projects/gmarkdown`. There was no git remotes yet; this repository is the salvage snapshot of that tree (build artifacts omitted).

## Goal

A lightweight GNOME Markdown viewer on FreeBSD that renders as close to GitHub as practical. Written in C with GTK4, libadwaita, and the system WebKitGTK 6.0 (`webkit2-gtk@60`). It should print/export nice PDFs, ship a GNOME-style icon, and be packaged as a FreeBSD port built in the usual local CircleCI (see `ehws-infra`). App id: `business.generic.GMarkdown` (generic.business), not any other domain.

## What was done

- C sources under `src/` (`main.c`, `window.c`, `markdown.c`, `print.c`) plus `meson.build`.
- GFM rendering via `cmark-gfm` (subproject wrap) and vendored highlight.js / mermaid (see `THIRD_PARTY.md`). Mermaid is meant to load only when a document has a mermaid fence.
- Desktop file, GResource, and hicolor icons (PNG sizes plus SVG) under `data/`.
- FreeBSD port stub in `packaging/freebsd/gmarkdown/` and `tools/ci-port.sh`.
- CircleCI config at `.circleci/config.yml` (`freebsd-15.1:current` / `freebsd.medium`, artifacts in `dist/*.pkg`).
- Design and rendering test document: `PLAN.md` (tables, alerts, Mermaid). `README.md` has the local build commands.

The Cursor session that wrote this ran late morning (America/Edmonton) on 2026-11-11. The last chat turn was still fetching dependencies while writing the app, icon, and port; the tree on disk already contains those files (timestamps through 11:13). A local `meson setup build` had been started on the laptop; that `build/` directory was intentionally left out of this snapshot.

## Where it stopped

Not confirmed as a completed, tested package. Next session should compile and run it on the laptop, use `PLAN.md` as the visual test, and try print-to-PDF. The Cursor CLI session later died in a reboot; there is no running agent to resume.

## How to build and run

```sh
pkg install meson ninja pkgconf gtk4 libadwaita webkit2-gtk@60 cmark-gfm
meson setup build
meson compile -C build
meson test -C build
./build/gmarkdown PLAN.md
```

Port/package:

```sh
sh tools/ci-port.sh
```

## Open TODOs / next steps

- Finish a clean local compile and fix anything the first `meson compile` / `meson test` surfaces.
- Confirm print-to-PDF and GitHub-like GFM coverage (alerts, tables, Mermaid, syntax highlighting).
- Register the project on the lab CircleCI and land the first `.pkg`.
- Open a PR once the port builds; this snapshot is work in progress, not a release.

## Decisions

- Name stays **gmarkdown** / **GMarkdown**. Public GitHub already has an unrelated project of the same name; Freshports had no `deskutils/gmarkdown`.
- Bundle id is `business.generic.GMarkdown`.
- License: BSD-2-Clause; vendored components keep upstream licenses.
