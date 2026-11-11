# GMarkdown

GitHub-style Markdown viewer for GNOME on FreeBSD. C, GTK4, libadwaita, WebKitGTK 6.0.

```sh
meson setup build
meson compile -C build
meson test -C build
./build/gmarkdown PLAN.md
```

`PLAN.md` is the design document and the rendering test (GFM, alerts, tables, Mermaid).

## Package

On FreeBSD, after deps:

```sh
pkg install meson ninja pkgconf gtk4 libadwaita webkit2-gtk@60 cmark-gfm
sh tools/ci-port.sh
```

CircleCI uses `freebsd-15.1:current` / `freebsd.medium` and stores `dist/*.pkg`.

App id: `business.generic.GMarkdown`.
