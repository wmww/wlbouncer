# wlbouncer
Control which Wayland protocol extensions are available to which clients on any Wayland compositor

## Features
- Supports yaml configuration (see [configuration.md](configuration.md))
- Can be used as a library by Wayland compositor developers
- Can be preloaded into any compositor without rebuilding it

## Dependencies
- wayland-server
- yaml-cpp

## Building & installing
- Install dependencies
- `meson setup build`
- `ninja -C build`
- `sudo ninja -C build install`

## Using as a user
- Run your Wayland compositor with `LD_PRELOAD` set to `/path/to/libwlbouncer-preload.so`
- ex: `LD_PRELOAD=/usr/local/lib/libwlbouncer-preload.so sway`

## Using as a compositor developer
- Link to the `wlbouncer` pkgconfig package
- Include `wlbouncer.h`
- Call `wl_bouncer_init_for_display()` after creating a Wayland display
- DO NOT call `wl_display_set_global_filter()` (it will replace wlbouncer's filter)

## Environment variables
- `BOUNCER_CONFIG`: set path to wlbouncer.yaml configuration file (can be overridden by compositor when wlbouncer is not preloaded)
- `BOUNCER_DEBUG`: if set to any value, wlbouncer will print what it's doing
- `BOUNCER_KEEP_LD_PRELOAD`: when wlbouncer is preloaded into a Wayland compositor it will clear `LD_PRELOAD` by default (so it's not preloaded into every descendant process of the compositor). Setting this to any value will prevent this behavior
