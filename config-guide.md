# BMCWeb Configuration Guide

This describes BMCWeb configuration items.

TODO: See the [OpenBMC Configuration Guide][1] for background.

[1]: https://github.com/openbmc/docs/config-guide.md

## Meta-configuration

BMCWeb is configured into OpenBMC by the `bmcweb` recipe.

## Build-time configuration

BMCWeb's configurable features and options are described in
`bmcweb/README.md` and `bmcweb/CMakeLists.txt`.

BMCWeb uses the CMake build tool which uses the configuration in
`bmcweb/CMakeLists.txt`.

The intended way to configure BMCWeb is to use `-D` flags to define
C++ preprocessor symbols for the desired configuration items.  These
symbols control which code is compiled into the program.

When `WEBSERVER_ENABLE_PAM` is used, BMCWeb installs Linux-PAM
configuration file `/etc/pam.d/webserver`.

## Run-time configuration

When BMCWeb starts running, it reads persistent configuration data
(such as UUID and session data) from `bmcweb_persistent_data.json`.
If the read fails, it generates a new a configuration and writes the
file.  If the data has changed when BMCWeb stops running, it writes
the file.

When BMCWeb starts running with `BMCWEB_ENABLE_SSL` enabled, it reads
its SSL certificate from `server.pem`.  If the read fails, it
re-generates the certificate and writes the file.
