# activities-overview — Project Brief

> GNOME Activities-style overview for MATE and the Marco window manager: press Super to zoom out all windows, switch workspaces, and type to search apps. Lightweight C/GTK3, X11, no Clutter.

This document covers the idea, scope and technical decisions behind the project. Read all of it before working on the code. Git workflow and setup are in `INIT_GITHUB.md`.

---

## 1. Motivation

MATE is much lighter than GNOME, but it has nothing like GNOME Shell's **Activities** overview (opened with the Super key). That single screen combines:

1. An overview of all open windows, shown with an animated zoom-out
2. Navigation between workspaces
3. App search by typing right away, without any click

For many users this feature is the only reason to stay on GNOME. The goal is to reproduce that behavior **as faithfully as possible** on MATE/Marco, without GNOME Shell's weight.

Alternatives considered:
- **xfdashboard**: the closest match, but it depends on Clutter (deprecated) and OpenGL, pulls in Xfce libraries (xfconf), and is slow to start.
- **Compiz Scale/Expo**: requires replacing Marco with Compiz.
- **skippy-xd / rofi**: each covers only part of it (exposé or launcher), not the integrated experience.

## 2. Goal

A lightweight daemon that opens a fullscreen overview when Super is tapped, behaving like GNOME Activities. It runs on top of Marco, or any EWMH-compliant X11 window manager with a compositor.

### Performance targets
| Metric | Target |
|---|---|
| Time to open (daemon already running) | < 100 ms to the first frame |
| Animation | 60 fps on an integrated Intel GPU |
| Idle RAM | < 30 MB |
| Idle CPU | ~0% (no polling, only X/D-Bus events) |

## 3. Expected behavior (reference: GNOME 3.3x / 40+)

### Open / close
- **Tapping** Super (press and release with no other key in between) toggles the overview.
- `Super + another key` must **not** trigger the overview, so existing shortcuts keep working.
- `Esc` clears the search text if there is any; otherwise it closes the overview.
- Clicking an empty area closes it.
- CLI: `activities-overview --toggle | --show | --hide`.

### Window view
- On open, every window on the current workspace **animates from its real position and size to its place in the grid** (~250 ms, ease-out-cubic). Closing plays the animation in reverse.
- Background: the current wallpaper, dimmed and/or blurred.
- **Live** thumbnails showing the real window contents, updated via XDamage.
- Hovering highlights the window and shows its title below and a close (X) button.
- Clicking activates the window and closes the overview with the animation.
- Middle click closes the window.
- Arrow keys / Tab move between windows; Enter activates.
- Minimized windows are shown too (cached thumbnail or the app icon).

### Workspaces
- A vertical strip on the right with a thumbnail of each workspace (wallpaper plus scaled windows).
- Clicking a thumbnail switches to that workspace; the overview stays open and shows its windows.
- Dragging a window from the grid onto a thumbnail moves it to that workspace.
- Scrolling over the strip switches workspaces.
- `Ctrl+Alt+↑/↓` (or PageUp/PageDown) switches workspaces while the overview is open.

### Search
- A search entry at the top, **focused automatically**, so you can just start typing.
- Typing replaces the window view with results:
  - **Applications** (.desktop files), ranked like GNOME does (`g_desktop_app_info_search`)
  - **Open windows** whose title or app name matches
- Enter opens the first result; arrow keys move through the results.
- Launching an app closes the overview.

### Multi-monitor
- The overview appears on every monitor, each showing the windows that are on it.
- Search and the workspace strip stay on the primary monitor.

## 4. Out of scope (for now)
- Wayland (MATE is effectively X11)
- File search, calculator and GNOME's other search providers
- Hot corner (can be done externally; to revisit later)
- Favorites dash and the "Show Applications" grid (phase 2)

## 5. Architecture

### Stack
| Component | Choice | Reason |
|---|---|---|
| Language | C11 | Standard in the MATE ecosystem, and the lightest option |
| UI / main loop | GTK3 (`GtkApplication`) | Single instance via D-Bus for free, CSS theming |
| Windows/workspaces | libwnck-3 | Mature EWMH abstraction (list, activate, move, close) |
| Thumbnails | XComposite + XDamage + XRender | Real window pixmaps, scaling accelerated via XRender |
| Rendering | cairo (`cairo_xlib_surface` over the pixmaps) | No Clutter, no direct OpenGL |
| Animation | `gtk_widget_add_tick_callback` (frame clock) | Synced to vblank |
| Super key | XInput2 **raw events** on the root window | Detects a tap without a grab, so `Super+X` shortcuts keep working |
| App search | GIO `GDesktopAppInfo` + `GAppInfoMonitor` | Native ranking, refreshes when apps are installed |
| Config | GSettings | MATE standard, editable via dconf |
| Build | Meson + Ninja | |

### Runtime model
- `activities-overview --daemon` starts via autostart (`/etc/xdg/autostart/activities-overview.desktop`), preloads state and stays idle.
- The overview windows (one per monitor: fullscreen, ARGB, override-redirect, always on top) are **created once and reused**; they are only mapped and unmapped.
- On open: snapshot window geometry from wnck → get pixmaps via `XCompositeNameWindowPixmap` → compute the layout → animate.
- While open: subscribe to XDamage only for visible windows. On close: release all damages and pixmaps.

### Compositor
- Live thumbnails require windows to be redirected: Marco's compositor turned on, or picom.
- Without a compositor, detect it (`_NET_WM_CM_S<n>`) and fall back to app icon plus title, with a warning in the log. Never force redirection.

### Grid layout
- Inspired by GNOME's `UnalignedLayoutStrategy`: pick the row count that maximizes the average scale, keep the relative order of windows by their original (x, y) position so the animation looks natural, and never scale a window above 100%.
- Implement it as a pure function (input: rectangles plus available area; output: rectangles) so it can be tested.

### Repository layout
```
activities-overview/
├── meson.build
├── meson_options.txt
├── data/
│   ├── activities-overview.desktop          # autostart
│   ├── io.github.guilhermepachecod.ActivitiesOverview.gschema.xml
│   └── style.css                            # default theme (GResource)
├── src/
│   ├── main.c              # GtkApplication, CLI, daemon lifecycle
│   ├── overview.c/.h       # overview window(s), show/hide, state
│   ├── window-grid.c/.h    # window grid, hover, click, DnD
│   ├── layout.c/.h         # layout algorithm (pure, testable)
│   ├── thumbnail.c/.h      # XComposite/XDamage/XRender → cairo
│   ├── workspaces.c/.h     # workspace strip
│   ├── search.c/.h         # entry, providers (apps, windows), results
│   ├── superkey.c/.h       # Super tap detection via XI2 raw events
│   ├── animation.c/.h      # interpolation/easing, tick callback
│   └── background.c/.h     # wallpaper (gsettings org.mate.background)
├── tests/
│   └── test-layout.c
├── debian/                 # .deb packaging
├── packaging/rpm/activities-overview.spec
├── .github/workflows/
│   ├── ci.yml              # build + tests on Debian 12, Debian 13, AlmaLinux 9
│   └── release.yml         # on v* tags: build .deb and .rpm, attach to the release
├── LICENSE                 # GPL-2.0-or-later
├── README.md
├── PROJECT.md
└── INIT_GITHUB.md
```

### Dependencies (Debian)
```
build-essential meson ninja-build pkg-config
libglib2.0-dev libgtk-3-dev libwnck-3-dev
libx11-dev libxcomposite-dev libxdamage-dev libxrender-dev libxi-dev libxfixes-dev
```
AlmaLinux 9: the matching `-devel` packages from EPEL (MATE itself comes from EPEL).

## 6. Roadmap

| Milestone | Deliverable |
|---|---|
| **M0** | Scaffold: meson, directory layout, CI building on all 3 distros, license, README |
| **M1** | Daemon + `--toggle` via D-Bus + fullscreen window with dimmed background + Esc closes |
| **M2** | Window grid with static thumbnails (pixmaps), layout, click activates |
| **M3** | Open/close animation from real geometry; live thumbnails (XDamage) |
| **M4** | Search: auto focus, apps + windows, full keyboard support |
| **M5** | Workspace strip: thumbnails, switching, scrolling, dragging windows |
| **M6** | Super tap via XI2 (detecting conflicts with MATE shortcuts) |
| **M7** | Multi-monitor |
| **M8** | Packaging: .deb, .rpm, automated GitHub releases |
| **M9** | Polish: GSettings (animation duration, dimming, etc.), CSS themes, no-compositor fallback, translations (gettext, pt_BR first) |

Each milestone must end with a warning-free build and something usable, even if incomplete.

## 7. Conventions

- Code, comments, commits, issues and docs **in English**.
- GLib/GNOME C style: `snake_case`, an `ao_` prefix on public functions, GObject only where it makes sense (widgets), plain structs elsewhere.
- CI builds with `-Wall -Wextra -Werror`; debug builds use `-fsanitize=address,undefined`.
- No new dependency without a justification.
- No polling: everything is event-driven (wnck signals, XDamage, D-Bus).
- Always release pixmaps and damages when the overview closes (check for leaks with valgrind/ASan).
- Small commits using Conventional Commits (`feat:`, `fix:`, `build:`...). See `INIT_GITHUB.md`.

## 8. MVP acceptance criteria (M1–M6)

- [ ] Tapping Super opens/closes the overview; `Super+D`, `Super+E`, etc. keep working
- [ ] Windows animate smoothly into the grid and back on close
- [ ] Clicking a window activates it; X closes it; middle click closes it
- [ ] Typing right after opening searches apps; Enter opens the first result
- [ ] The workspace strip switches workspaces and accepts dragged windows
- [ ] Opens in < 100 ms with the daemon running, on a laptop with an integrated Intel GPU
- [ ] Works on Debian 12/13 with MATE + Marco (compositor on)

## 9. Manual reference testing

Main environment: Debian 12/13 with MATE and Marco's compositor turned on
(`gsettings set org.mate.Marco.general compositing-manager true`).
Compare side by side with GNOME on another machine to check that behavior and timing match.