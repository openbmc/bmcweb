# BMCWeb Configuration Guide

This describes BMCWeb configuration items.

TODO: See the [OpenBMC Configuration Guide][1] for background.

[1]: https://github.com/openbmc/docs/config-guide.md

## Build-time configuration

BMCWeb is configured by setting `-D` flags that correspond to options
in `bmcweb/CMakeLists.txt` and then re-compiling.  For example, `cmake
-DBMCWEB_ENABLE_KVM=NO ...`.  The option names become C++ preprocessor
symbols that control which code is compiled into the program.

## Run-time configuration

When BMCWeb starts running, it reads persistent configuration data
(such as UUID and session data) from `bmcweb_persistent_data.json`.
If the read fails, it generates a new a configuration and writes the
file.  If the data has changed when BMCWeb stops running, it writes
the file.

When BMCWeb with `BMCWEB_ENABLE_SSL` enabled starts running, it reads
its certificate from a file.  If the read fails, it generates RSA keys
and certificates using OpenSSL, and writes the file.
