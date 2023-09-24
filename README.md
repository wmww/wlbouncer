# Wl Bouncer
Allows controlling which Wayland protocols are available to which clients on any Wayland compositor

Install `wayland-utils` and run `wayland-info | grep -Po "(?<=interface: ')\w*"` to get a list of available Wayland extensions.
