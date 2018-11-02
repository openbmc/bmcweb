# BMCWeb Configuration Guide

This describes BMCWeb configuration items.

## BMCWeb meta-configuration

The BMCWeb meta-configuration items in this section are in BitBake
recipes and appends in the `openbmc` repository under various `meta-`
subtrees.

BMCWeb is built into an OpenBMC image by setting `RDEPENDS = "bmcweb"`
in a recipe, typically in a distribution's packagegroup recipe.

Recipe `meta-phosphor/recipes-phosphor/interfaces/bmcweb_git.bb`
builds the BMCWeb package and controls items like:
 - the URL and version of bmcweb to use
 - the systemd config files for the bmcweb service
 - the userid for bmcweb to run under

BMCWeb is designed so its features (described below) can be selected
by setting `EXTRA_OECMAKE` flags in bmcweb `.bbappend` files.

## BMCWeb build-time configuration

The BMCWeb configuration items in this section are in the `bmcweb`
repository.

BMCWeb uses the CMake build tool which is configured by
`bmcweb/CMakeLists.txt` and controls items like:
 - C++ compiler version and options
 - PAM configuration files
 - bmcweb features (settings.hpp.in)

BMCWeb's features and options are described in `bmcweb/README.md` and
`bmcweb/CMakeLists.txt`.  Flags in `bmcweb/settings.hpp.in` control
which features and options are used.  Note that BMCWeb does not read a
config file when it starts running.

The features are:
TODO: I stopped here for now ...  I will follow up with basic descriptions for these

#cmakedefine BMCWEB_ENABLE_KVM
#cmakedefine BMCWEB_ENABLE_DBUS_REST
#cmakedefine BMCWEB_ENABLE_REDFISH
#cmakedefine BMCWEB_ENABLE_STATIC_HOSTING
#cmakedefine BMCWEB_ENABLE_HOST_SERIAL_WEBSOCKET
#cmakedefine BMCWEB_INSECURE_DISABLE_CSRF_PREVENTION
#cmakedefine BMCWEB_INSECURE_DISABLE_SSL
#cmakedefine BMCWEB_INSECURE_DISABLE_XSS_PREVENTION
#cmakedefine BMCWEB_ENABLE_REDFISH_RAW_PECI
#cmakedefine BMCWEB_ENABLE_REDFISH_CPU_LOG
??? more from the CMakeLists.txt ???

### BMCWEB_ENABLE_KVM

Feature BMCWEB_ENABLE_KVM enables the KVM websocket path `/kvmws`, with
details in `bmcweb/include/web_kvm.hpp` template function
`crow::kvm::requestRoutes`.
TODO: Affected by BMCWEB_ENABLE_SSL ?


TODO: For each feature, describe how to find
 - protocols (https, wss, etc)
 - TCP (and UDP?) port numbers
 - URIs

TODO: For