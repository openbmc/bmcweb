# BMCWeb Configuration Guide

This describes BMCWeb configuration items.

TODO: See the [OpenBMC Configuration Guide
Primer](https://github.com/openbmc/docs/config-guide-primer.md) for
background information.

## Meta-configuration

OpenBMC `bmcweb` recipes are used to bake BMCWeb.

## Build-time configuration

BMCWeb's configurable features and options are described in
`bmcweb/README.md` and `bmcweb/CMakeLists.txt`.  The intended way to
configure BMCWeb is to set the CMAKE variable (via EXTRA_OECMAKE) to
define C++ preprocessor symbols which correspond to the desired
configuration.  The preprocessor symbols then control which code is
compiled into the program.

BMCWeb uses the CMake build tool which uses the configuration in
`bmcweb/CMakeLists.txt`.

## Run-time configuration

When `WEBSERVER_ENABLE_PAM` is used, BMCWeb provides PAM configuration
file `/etc/pam.d/webserver`.

When `BMCWEB_ENABLE_SSL` is used, BMCWeb reads its SSL certificate
from `server.pem`.  If that fails, it re-generates the certificate and
writes its file.
