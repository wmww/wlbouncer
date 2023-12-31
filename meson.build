project('wlbouncer',
    ['cpp'],
    version: '0.1.0',
    license: 'MIT',
    meson_version: '>=0.51.0',
    default_options: ['cpp_std=c++20', 'warning_level=3'])

pkg_config = import('pkgconfig')
wayland_server = dependency('wayland-server')
yaml_cpp = dependency('yaml-cpp')

srcs = files(
    'src/listeners.cpp',
    'src/policy.cpp')

install_headers('include/wlbouncer.h')

wl_bouncer_lib = library('wlbouncer',
    srcs,
    include_directories: include_directories('include'),
    dependencies: [wayland_server, yaml_cpp],
    soversion: 0,
    install: true)

pkg_config.generate(
    name: 'wlbouncer',
    version: meson.project_version(),
    libraries: [wl_bouncer_lib],
    filebase: 'wlbouncer',
    description: 'Control which Wayland protocol extensions are available to which clients on any Wayland compositor',
    url: 'https://github.com/wmww/wlbouncer')

wl_bouncer_preload_lib = library('wlbouncer-preload',
    srcs + files('src/libwayland-shim.cpp'),
    include_directories: include_directories('include'),
    dependencies: [wayland_server, yaml_cpp],
    install: true)
