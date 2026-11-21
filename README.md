# activities-overview

> GNOME Activities-style overview for MATE and the Marco window manager: press Super to zoom out all windows, switch workspaces, and type to search apps. Lightweight C/GTK3, X11, no Clutter.

**Status: work in progress.** See [`PROJECT.md`](PROJECT.md) for the full project brief, architecture, and roadmap.

## Dependencies (Debian)

```
build-essential meson ninja-build pkg-config
libglib2.0-dev libgtk-3-dev libwnck-3-dev
libx11-dev libxcomposite-dev libxdamage-dev libxrender-dev libxi-dev libxfixes-dev
```

AlmaLinux 9: `-devel` equivalents via EPEL (MATE ships from EPEL).

## Build

```bash
meson setup build
meson compile -C build
meson test -C build
```

## License

GPL-2.0-or-later. See [`LICENSE`](LICENSE).
