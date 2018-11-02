# BMCWeb Configuration Guide

This describes BMCWeb configuration items.

TODO: See the [OpenBMC Configuration Guide
Primer](https://github.com/openbmc/docs/config-guide-primer.md) for
background information.

## Meta-configuration

BMCWeb is provided by the
`meta-phosphor/recipes-phosphor/interfaces/bmcweb_git.bb` recipe which
builds the `bmcweb` package and controls items like:
 - the version of bmcweb to use
 - the systemd config files for the bmcweb service
 - the userid for bmcweb to run under

## Build-time configuration

BMCWeb uses the CMake build tool which uses the configuration in
`bmcweb/CMakeLists.txt` to control items like:
 - which C++ compiler version and options to use
 - which bmcweb features and options to use
 - compile time (link time) dependencies

BMCWeb's features and options are described in `bmcweb/README.md` and
`bmcweb/CMakeLists.txt` and controlled by flags in
`bmcweb/settings.hpp.in`.  The option names become C++ preprocessor
symbols which control which code gets compiled into the program.

The BMCWeb program does not read a configuration file.
