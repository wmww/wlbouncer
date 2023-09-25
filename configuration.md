# Configuring wlbouncer
wlbouncer is configured with a YAML configuration file. If the `BOUNCER_CONFIG` environment variable is set, it is expected to contain the path of the config file. Otherwise the following paths are searched in order:
- `/usr/local/etc/wlbouncer.yaml`
- `/etc/wlbouncer.yaml`
- `$HOME/.config/wlbouncer.yaml`
If wlbouncer is being used for security keeping the config file in your home directory may not be desirable.

The config file should contain a version (currently must be `0`) and a `policy` that contains a list of directives. Here's an example demonstrating many of the features:
```yaml
version: 0
policy:
  # Disable zxdg_decoration_manager_v1 in all cases, unless something below enables it
  - disable: zxdg_decoration_manager_v1

  # Enable all protocol extensions for clients run by root except primary selection and data control
  - user: root
    disable-only:
      - zwp_primary_selection_device_manager_v1
      - zwlr_data_control_manager_v1

  # Enable all protocol extensions for clients run by root or alice if their PID is the same as the compositors's PID
  # (this is useful for allowing desktop environment components started by the Wayland compositor)
  - pid: $SELF_PID
    users: [root, alice]
    enable: all
```

## Directives
Each directive contains zero or more conditions (all of which must be met for the directive to apply) and a list of Wayland global names to enable or disable.

```yaml
# For clients run by the root user
user: root
# Disable these extensions (and make all others enabled)
disable-only:
  - zwp_primary_selection_device_manager_v1
  - zwlr_data_control_manager_v1
```

## Conditions
Conditions check a property of the connecting client. Each condition has different variables that can be used instead of a hard-coded value.

- `pid`: the process ID of the connecting client, generally the only sensible values to use for this are variables.
  - `$SELF_PID`: the PID of the Wayland compositor (desktop environment components will often report the same PID as the compositor)
  - `$PARENT_PID`: the PID of the Wayland compositor's parent process
- `uid`: the user ID of the connecting client (a number)
  - `$SELF_UID`: the real user ID of the Wayland compositor
  - `$SELF_EUID`: the effective user ID of the Wayland compositor (see [this](https://en.wikipedia.org/wiki/User_identifier) for distinction between real user ID and effective user ID)
- `gid`: the group ID of the connecting client (a number)
  - `$SELF_GID`: the real group ID of the Wayland compositor
  - `$SELF_EGID`: the effective group ID of the Wayland compositor
- `user`: the username of the user that's running the connecting client
  - `$SELF_USER`: the username of the real user that's running the Wayland compositor
  - `$SELF_EUSER`: the username of the effective user that's running the Wayland compositor
- `group`: the group name of the user that's running the connecting client
  - `$SELF_USER`: the group name of the real user that's running the Wayland compositor
  - `$SELF_EUSER`: the group name of the effective user that's running the Wayland compositor

## Wayland global lists
To get a list of Wayland globals that are currently exposed, install the `wayland-utils` package and run `wayland-info | grep -Po "(?<=interface: ')\w*"`.

There are four types of Wayland global lists:
- `enable`: expose these Wayland globals to the client, does not effect globals not mentioned
- `disable`: do not expose these Wayland globals to the client, does not effect globals not mentioned
- `enable-only`: expose these Wayland globals to the client, do not expose any others
- `disable-only`: expose all Wayland globals to the client except these

Note that whether a global is exposed or not can be overridden by any matching directive further down in the configuration file.

The value of the global list can be a list of globals, a single global or `all` (which is essentially a wildcard).

## Defaults
By default, a whitelist of generally harmless and essential globals are enabled. You can find that list [at the top of polciy.cpp](src/policy.cpp). To instead start off with all protocols enabled, simply make the first directive in your config file an unconditional:
```yaml
- enable: all
```
