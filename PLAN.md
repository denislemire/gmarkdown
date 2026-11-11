# GMarkdown

A GTK4 / libadwaita Markdown **viewer** for FreeBSD GNOME. It is not an editor. Open a `.md` file and read it the way GitHub would, then print or export a PDF.

This document is the design **and** the capability test. Open it with:

```sh
gmarkdown PLAN.md
```

App id: `business.generic.GMarkdown`  
Binary: `gmarkdown`  
Homepage: <https://generic.business>

> [!NOTE]
> Reverse-DNS of **generic.business**, not incoherency.net.

> [!TIP]
> On hpbsd, WebKit needs `WEBKIT_DISABLE_COMPOSITING_MODE=1`. The app sets that unless it is already in the environment.

> [!WARNING]
> The N150 laptop has 4 GB RAM. One WebView, no extra browser, no network inside the page.

> [!IMPORTANT]
> CircleCI on EhWS builds the `.pkg`. Local meson must already compile and pass `meson test`.

> [!CAUTION]
> Do not compile WebKit in CI. Fetch `webkit2-gtk@60` from official latest.

---

## Why this stack

GitHub Flavored Markdown is defined by **cmark-gfm**. The page is HTML plus GitHub-like CSS, shown in the same **WebKitGTK 6.0** engine already on hpbsd (`webkit2-gtk_60` 2.46, GTK 4.20, libadwaita 1.8). Print goes through `WebKitPrintOperation` into CUPS (Xerox B210 in the lab).

| Piece | Choice | Why |
| --- | --- | --- |
| Language | C11 | Matches bsd-cubed / native WebKit probes |
| UI | GTK4 + libadwaita | GNOME 49 on hpbsd |
| Engine | webkitgtk-6.0 | Same as Epiphany on this system |
| Parser | cmark-gfm | GitHub's fork, not stock cmark |
| Diagrams | mermaid.min.js | GitHub ` ```mermaid ` fences |
| Highlight | highlight.js | GitHub light/dark themes |
| Package | `deskutils/gmarkdown` | bsd-cubed overlay style |
| CI | `freebsd.medium` | EhWS KubeVirt, not poudriere |

- [x] GFM tables, task lists, strikethrough, autolink, tagfilter
- [x] Footnotes and GitHub alerts
- [x] Mermaid fences
- [x] Print and export PDF
- [ ] Math / KaTeX (out of v1)
- [ ] In-place editing
- [ ] Flatpak

Hello **bold**, *italic*, ~~strikethrough~~, `inline code`, and a link to [generic.business](https://generic.business).

Autolink: https://circle.ehws.generic.business

---

## Pipeline

```mermaid
flowchart LR
  mdFile[Markdown file] --> monitor[GFileMonitor]
  mdFile --> cmark[cmark-gfm]
  cmark --> alerts[Alert rewrite]
  alerts --> mermaidFence[Mermaid fence rewrite]
  mermaidFence --> wrap[GitHub CSS wrap]
  wrap --> webkit[WebKitGTK 6.0]
  webkit --> window[AdwApplicationWindow]
  webkit --> printOp[WebKitPrintOperation]
  printOp --> cups[CUPS Xerox-B210]
  printOp --> pdf[Export PDF]
  monitor -->|"save on disk"| cmark
```

Relative images resolve against the file directory (`file:` base URI). `http(s)` and `mailto:` clicks leave the WebView through `GtkUriLauncher`.

---

## Application layout

```mermaid
flowchart TB
  app[AdwApplication business.generic.GMarkdown]
  app --> win[GMarkdownWindow]
  win --> header[AdwHeaderBar]
  win --> find[Find revealer]
  win --> split[AdwOverlaySplitView]
  split --> toc[Contents sidebar]
  split --> view[WebKitWebView]
  view --> scheme[gmarkdown://bundle]
  scheme --> gres[GResource CSS/JS]
  scheme --> mermaidJs[pkgdata mermaid.min.js]
```

Keyboard: <kbd>Ctrl</kbd>+<kbd>O</kbd> open, <kbd>Ctrl</kbd>+<kbd>F</kbd> find, <kbd>F9</kbd> contents, <kbd>Ctrl</kbd>+<kbd>P</kbd> print, <kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>P</kbd> export PDF, zoom with <kbd>Ctrl</kbd>+<kbd>+</kbd> / <kbd>-</kbd> / <kbd>0</kbd>.

Drop a `.md` file on the window. Save it in another editor and the view reloads.

---

## GitHub-flavored details

cmark-gfm options: footnotes, GitHub info strings on code fences, smart punctuation, UTF-8 validation. Extensions: table, strikethrough, autolink, tagfilter, tasklist.

Alerts are GFM blockquotes starting with `[!NOTE]` and friends; we rewrite those into `.markdown-alert` nodes after HTML render.

Heading ids follow GitHub slugs so the contents sidebar can `scrollIntoView`.

```c
int
options(void)
{
	return (CMARK_OPT_FOOTNOTES |
	    CMARK_OPT_GITHUB_PRE_LANG |
	    CMARK_OPT_SMART);
}
```

```python
def greet(name: str) -> str:
    return f"hello {name}"
```

```js
mermaid.initialize({ startOnLoad: false, securityLevel: "strict" });
```

A footnote with a longer body.[^hpbsd]

[^hpbsd]: hpbsd is the HP N150 laptop running FreeBSD 15 and GNOME 49. Lab notes live in `hpbsd-lab`, not a `~/Projects/hpbsd` tree.

---

## Sequence: open a file

```mermaid
sequenceDiagram
  actor User
  participant App as gmarkdown
  participant Parser as cmark-gfm
  participant WK as WebKitGTK
  participant CUPS as CUPS
  User->>App: gmarkdown PLAN.md
  App->>Parser: GFM + extensions
  Parser-->>App: HTML
  App->>WK: load_html + file base URI
  WK-->>User: GitHub-like page
  User->>App: Ctrl+P
  App->>WK: WebKitPrintOperation
  WK->>CUPS: print job
```

---

## Class sketch

```mermaid
classDiagram
  class GMarkdownWindow {
    +WebKitWebView view
    +GFile file
    +open_file(GFile)
  }
  class GMarkdownDocument {
    +char body_html
    +GPtrArray headings
    +gboolean has_mermaid
  }
  class WebKitPrintOperation {
    +run_dialog()
    +print()
  }
  GMarkdownWindow --> GMarkdownDocument : parse
  GMarkdownWindow --> WebKitPrintOperation : print / PDF
```

---

## State: viewing

```mermaid
stateDiagram-v2
  [*] --> Empty: launch
  Empty --> Loaded: open or drop
  Loaded --> Loaded: file monitor
  Loaded --> Printing: Ctrl+P
  Printing --> Loaded: dialog closes
  Loaded --> Exporting: export PDF
  Exporting --> Loaded: print-to-file
```

---

## Packaging and CI

```mermaid
flowchart TB
  src[git checkout]
  src --> meson[meson setup compile test]
  src --> overlay[packaging/freebsd/gmarkdown]
  meson --> ciPort[tools/ci-port.sh]
  overlay --> ciPort
  ciPort --> tarball[snapshot distfile]
  tarball --> portsMake[makesum stage test check-plist stage-qa package]
  portsMake --> artifact[dist/*.pkg]
  artifact --> onboard[circle.ehws.generic.business]
```

CI machine: image `freebsd-15.1:current`, class `freebsd.medium`. Fetch gtk4, libadwaita, webkit2-gtk@60, cmark-gfm from official latest. Do **not** publish automatically to `packages.ehws.generic.business`.

ER of the lab pieces this app cares about:

```mermaid
erDiagram
  HPBSD ||--|| WEBKIT : runs
  HPBSD ||--|| CUPS : prints
  EHWS ||--o{ CIRCLECI : hosts
  CIRCLECI ||--o{ FREEBSD_VM : freebsd.medium
  FREEBSD_VM ||--o{ GMARKDOWN_PKG : builds
```

---

## Gantt (build, not a product roadmap)

```mermaid
gantt
  title First package
  dateFormat  YYYY-MM-DD
  section App
  C viewer and GFM     :a1, 2026-11-11, 1d
  Print and PDF        :a2, after a1, 1d
  section Ship
  FreeBSD port         :p1, after a1, 1d
  CircleCI job         :p2, after p1, 1d
```

---

## What v1 refuses to be

```mermaid
pie showData
  title Process budget on 4 GB
  "WebKit web process" : 55
  "GTK UI" : 15
  "cmark-gfm" : 5
  "highlight.js" : 10
  "mermaid.js when used" : 15
```

Mermaid loads **only** when the document contains a mermaid fence, so ordinary README files stay lighter.

---

## License

BSD-2-Clause for GMarkdown. Vendored cmark-gfm, highlight.js, and mermaid keep their upstream licenses; see `THIRD_PARTY.md`.
