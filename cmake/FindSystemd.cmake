include(FeatureSummary)
set_package_properties(Systemd PROPERTIES
   URL "http://freedesktop.org/wiki/Software/systemd/"
   DESCRIPTION "System and Service Manager")

find_package(PkgConfig)
pkg_check_modules(PC_SYSTEMD libsystemd)
find_library(SYSTEMD_LIBRARIES NAMES systemd ${PC_SYSTEMD_LIBRARY_DIRS})
find_path(SYSTEMD_INCLUDE_DIRS systemd/sd-login.h HINTS ${PC_SYSTEMD_INCLUDE_DIRS})

set(SYSTEMD_DEFINITIONS ${PC_SYSTEMD_CFLAGS_OTHER})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SYSTEMD DEFAULT_MSG SYSTEMD_INCLUDE_DIRS SYSTEMD_LIBRARIES)
mark_as_advanced(SYSTEMD_INCLUDE_DIRS SYSTEMD_LIBRARIES SYSTEMD_DEFINITIONS)
