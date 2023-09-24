project('wlbouncer',
    ['cpp'],
    version: '1.0.0',
    license: 'MIT',
    meson_version: '>=0.51.0',
    default_options: ['cpp_std=c++20', 'warning_level=3'])

wayland_server = dependency('wayland-server', version: '>=1.10.0')

srcs = files(
    'src/listeners.cpp',
    'src/policy.cpp')

install_headers('include/wlbouncer.h')

wl_bouncer_lib = library('wlbouncer',
    srcs,
    include_directories: include_directories('include'),
    dependencies: [wayland_server],
    install: true)

wl_bouncer_preload_lib = library('wlbouncer-preload',
    srcs + files('src/libwayland-shim.cpp'),
    include_directories: include_directories('include'),
    dependencies: [wayland_server],
    install: true)
