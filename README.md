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

Build the local deskutils/gmarkdown port as an ordinary user. Set PORTSDIR to use an existing FreeBSD ports tree outside /usr/ports.

```sh
sudo pkg install meson ninja pkgconf gtk4 libadwaita webkit2-gtk_60 cmark-gfm desktop-file-utils git
sh tools/ci-port.sh
sudo pkg add ./dist/packages/All/gmarkdown-0.1.0_1.pkg
```

CircleCI uses `freebsd-15.1:current` / `freebsd.medium` and stores `dist/packages/All/*.pkg`.

App id: `business.generic.GMarkdown`.
