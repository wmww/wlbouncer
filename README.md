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

## Using as a compositor developer
- TODO

## Using as a user
- Run your Wayland compositor with `LD_PRELOAD` set to `/path/to/libwlbouncer-preload.so`
- ex: `LD_PRELOAD=/usr/local/lib/libwlbouncer-preload.so sway`

## Environment variables
- TODO
- BOUNCER_KEEP_LD_PRELOAD
