cmake_minimum_required (VERSION 3.5 FATAL_ERROR)

project (bmc-webserver CXX)

cmake_policy (SET CMP0054 NEW)

set (CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

option (BUILD_STATIC_LIBS "Built static libraries" ON)

option (YOCTO_DEPENDENCIES "Use YOCTO dependencies system" OFF)

option (
    BMCWEB_ENABLE_KVM
    "Enable the KVM host video WebSocket.  Path is '/kvm/0'.  Video is from the
    BMC's '/dev/video' device."
    ON
)
option (
    BMCWEB_ENABLE_VM_WEBSOCKET
    "Enable the Virtual Media WebSocket. Path is '/vm/0/0'to open the websocket.
    See https://github.com/openbmc/jsnbd/blob/master/README."
    ON
)
option (
    BMCWEB_ENABLE_VM_NBDPROXY
    "Enable the Virtual Media WebSocket."
    OFF
)
option (
    BMCWEB_ENABLE_DBUS_REST
    "Enable Phosphor REST (D-Bus) APIs.  Paths directly map Phosphor D-Bus
    object paths, for example, '/xyz/openbmc_project/logging/entry/enumerate'.
    See https://github.com/openbmc/docs/blob/master/rest-api.md."
    ON
)
option (
    BMCWEB_ENABLE_REDFISH
    "Enable Redfish APIs.  Paths are under '/redfish/v1/'.  See
    https://github.com/openbmc/bmcweb/blob/master/DEVELOPING.md#redfish."
    ON
)
option (
    BMCWEB_ENABLE_HOST_SERIAL_WEBSOCKET
    "Enable host serial console WebSocket.  Path is '/console0'.  See
    https://github.com/openbmc/docs/blob/master/console.md."
    ON
)
option (
    BMCWEB_ENABLE_STATIC_HOSTING
    "Enable serving files from the '/usr/share/www' directory as paths under
    '/'."
    ON
)
option (
    BMCWEB_ENABLE_REDFISH_BMC_JOURNAL
    "Enable BMC journal access through Redfish.  Paths are under
    '/redfish/v1/Managers/bmc/LogServices/Journal'."
    OFF
)
option (
    BMCWEB_ENABLE_REDFISH_RAW_PECI
    "Enable PECI transactions through Redfish.  Paths are under
    '/redfish/v1/Systems/system/LogServices/CpuLog/Actions/Oem/CpuLog.SendRawPeci'."
    OFF
)
option (
    BMCWEB_ENABLE_REDFISH_CPU_LOG
    "Enable CPU log service transactions through Redfish.  Paths are under
    '/redfish/v1/Systems/system/LogServices/Crashdump'."
    OFF
)
option (
    BMCWEB_ENABLE_REDFISH_DUMP_LOG
    "Enable BMC and System dump log service transactions through Redfish. For BMC dump, paths
    are under '/redfish/v1/Managers/bmc/LogServices/Dump' and for System dump,
    paths are under '/redfish/v1/Systems/system/LogServices/Dump'."
    OFF
)
option (
    BMCWEB_ENABLE_REDFISH_DBUS_LOG_ENTRIES
    "Enable DBUS log service transactions through Redfish. Paths are under
    '/redfish/v1/Systems/system/LogServices/EventLog/Entries'."
    OFF
)
option (
    BMCWEB_ENABLE_REDFISH_PROVISIONING_FEATURE
    "Enable provisioning feature support in redfish. Paths are under
    '/redfish/v1/Systems/system/'."
    OFF
)
option (
    BMCWEB_ENABLE_MUTUAL_TLS_AUTHENTICATION
    "Enables authenticating users through TLS client certificates.
    The BMCWEB_INSECURE_DISABLE_SSL must be OFF for this option to take effect."
    ON
)
option (
    BMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE
    "Enable the IBM management console specific functionality. Paths are under
    '/ibm/v1/'."
    OFF
)


# Insecure options.  Every option that starts with a BMCWEB_INSECURE flag should
# not be enabled by default for any platform, unless the author fully
# comprehends the implications of doing so.  In general, enabling these options
# will cause security problems of varying degrees
option (
    BMCWEB_INSECURE_DISABLE_CSRF_PREVENTION
    "Disable CSRF prevention checks. Should be set to OFF for production
    systems."
    OFF
)

option (BMCWEB_INSECURE_DISABLE_SSL
        "Disable SSL ports. Should be set to OFF for production systems." OFF)

option (
    BMCWEB_INSECURE_DISABLE_AUTHENTICATION
    "Disable authentication on all ports. Should be set to OFF for production
    systems"
    OFF
)

option (BMCWEB_INSECURE_DISABLE_XSS_PREVENTION "Disable XSS preventions" OFF)

option (
    BMCWEB_INSECURE_ENABLE_REDFISH_FW_TFTP_UPDATE
    "Enable TFTP based firmware update transactions through Redfish
    UpdateService.SimpleUpdate."
    OFF
)

option (
    BMCWEB_INSECURE_ENABLE_HTTP_PUSH_STYLE_EVENTING
    "Enable HTTP push style eventing feature" OFF
)

option (
    BMCWEB_ENABLE_VALIDATION_UNSECURE_FEATURE
    "Enables unsecure features required by validation. Note: must
    be turned off for production images."
    OFF)

option (
    BMCWEB_INSECURE_UNRESTRICTED_SENSOR_OVERRIDE
    "Enables Sensor override feature without any check."
    OFF)

set (BMCWEB_HTTP_REQ_BODY_LIMIT_MB "30" CACHE STRING
     "The max HTTP request body size in MB")

configure_file(config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/include/config.h)

if (BMCWEB_ENABLE_MUTUAL_TLS_AUTHENTICATION AND BMCWEB_INSECURE_DISABLE_SSL)
    message("SSL Must be enabled to allow SSL authentication")
    set(BMCWEB_ENABLE_MUTUAL_TLS_AUTHENTICATION OFF)
endif()


include (CTest)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

set (CMAKE_EXPORT_COMPILE_COMMANDS ON)

set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -Wall")

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
    -fno-rtti \
")

set (
    CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} \
    -Wall \
    -Wextra \
    -Wnon-virtual-dtor \
    -Wold-style-cast \
    -Wcast-align \
    -Wunused \
    -Woverloaded-virtual \
    -Wpedantic \
    -Wconversion \
    -Wsign-conversion \
    -Wnull-dereference \
    -Wdouble-promotion \
    -Wformat=2 \
    -Wno-unused-parameter \
"
)

# only set -Werror if we're on a compiler that we know passes
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 8.0)
        set (
            CMAKE_CXX_FLAGS
            "${CMAKE_CXX_FLAGS} \
            -Werror \
            -Wduplicated-cond \
            -Wduplicated-branches \
            -Wlogical-op \
            -Wnull-dereference \
            -Wdouble-promotion \
            -Wformat=2 \
            -Wno-unused-parameter \
        "
        )
    endif (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 8.0)
endif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 9.0)
        set (
            CMAKE_CXX_FLAGS
            "${CMAKE_CXX_FLAGS} \
            -Werror \
            -Weverything \
            -Wno-c++98-compat \
            -Wno-c++98-compat-pedantic \
            -Wno-global-constructors \
            -Wno-exit-time-destructors \
            -Wcovered-switch-default \
            -Wno-shadow \
            -Wno-used-but-marked-unused \
            -Wcomma \
            -Wextra-semi \
            -Wno-documentation-unknown-command \
            -Wno-weak-vtables \
            -Wno-documentation \
            -Wno-padded \
            -Wzero-as-null-pointer-constant \
            -Wswitch-enum \
            -Wnull-dereference \
            -Wdouble-promotion \
            -Wformat=2 \
            -Wno-unused-parameter \
        "
        )
    endif (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 8.0)
endif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-rtti")

# general
option (BMCWEB_BUILD_UT "Enable Unit test" OFF)

# security flags
set (
    SECURITY_FLAGS
    "-fstack-protector-strong \
    -fPIE \
    -fPIC \
    -D_FORTIFY_SOURCE=2 \
    -Wformat \
    -Wformat-security"
)
set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${SECURITY_FLAGS}")
set (CMAKE_CXX_FLAGS_RELWITHDEBINFO
     "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${SECURITY_FLAGS}")
set (CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} ${SECURITY_FLAGS}")

# Enable link time optimization This is a temporary workaround because
# INTERPROCEDURAL_OPTIMIZATION isn't available until cmake 3.9. gcc-ar and gcc-
# ranlib are wrappers around ar and ranlib which add the lto plugin to the
# command line.
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if (NOT CMAKE_BUILD_TYPE MATCHES Debug)
        string (REGEX REPLACE "ar$" "gcc-ar" CMAKE_AR ${CMAKE_AR})
        string (REGEX
                REPLACE "ranlib$" "gcc-ranlib" CMAKE_RANLIB ${CMAKE_RANLIB})
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto -fno-fat-lto-objects")

        # Reduce the binary size by removing unnecessary dynamic symbol table
        # entries
        set (
            CMAKE_CXX_FLAGS
            "${CMAKE_CXX_FLAGS} \
        -fvisibility=hidden \
        -fvisibility-inlines-hidden \
        -Wl,--exclude-libs,ALL"
        )
    endif (NOT CMAKE_BUILD_TYPE MATCHES Debug)
endif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")

if (NOT ${YOCTO_DEPENDENCIES}) # Download and unpack googletest at configure
                               # time
    configure_file (CMakeLists.txt.in 3rdparty/CMakeLists.txt)
    execute_process (COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
                     WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/3rdparty)
    execute_process (COMMAND ${CMAKE_COMMAND} --build .
                     WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/3rdparty)

    set (CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR}/prefix ${CMAKE_PREFIX_PATH})
endif ()

find_package (Boost 1.73 REQUIRED)
message (BOOST_VERSION = ${Boost_VERSION})
include_directories (SYSTEM ${BOOST_SRC_DIR})

# add_definitions(-DBOOST_ASIO_ENABLE_HANDLER_TRACKING)
add_definitions (-DBOOST_ASIO_DISABLE_THREADS)
add_definitions (-DBOOST_BEAST_USE_STD_STRING_VIEW)
add_definitions (-DBOOST_ERROR_CODE_HEADER_ONLY)
add_definitions (-DBOOST_SYSTEM_NO_DEPRECATED)
add_definitions (-DBOOST_ASIO_NO_DEPRECATED)
add_definitions (-DBOOST_ALL_NO_LIB)
add_definitions (-DBOOST_NO_RTTI)
add_definitions (-DBOOST_NO_TYPEID)
add_definitions (-DBOOST_COROUTINES_NO_DEPRECATION_WARNING)

# sdbusplus
if (NOT ${YOCTO_DEPENDENCIES})
    include_directories (SYSTEM ${CMAKE_BINARY_DIR}/sdbusplus-src)
    link_directories (${CMAKE_BINARY_DIR}/sdbusplus-src/build)
endif ()

# Its an Out of tree build,enabling ibm management console for
# unit-test purpose.
if (NOT ${YOCTO_DEPENDENCIES})
    add_definitions(-DBMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE)
endif(NOT ${YOCTO_DEPENDENCIES})

# Openssl
find_package (OpenSSL REQUIRED)
include_directories (SYSTEM ${OPENSSL_INCLUDE_DIR})
message ("OPENSSL_INCLUDE_DIR ${OPENSSL_INCLUDE_DIR}")

# bmcweb
message ("CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
if (CMAKE_BUILD_TYPE MATCHES Debug)
    message ("Logging enabled")
    add_definitions (-DBMCWEB_ENABLE_LOGGING)
    add_definitions (-DBMCWEB_ENABLE_DEBUG)
endif (CMAKE_BUILD_TYPE MATCHES Debug)

if (NOT "${BMCWEB_INSECURE_DISABLE_SSL}")
    add_definitions (-DBMCWEB_ENABLE_SSL)
endif (NOT "${BMCWEB_INSECURE_DISABLE_SSL}")
include_directories (${CMAKE_CURRENT_SOURCE_DIR}/http)

# Zlib
find_package (ZLIB REQUIRED)
include_directories (SYSTEM ${ZLIB_INCLUDE_DIRS})

# PAM
option (WEBSERVER_ENABLE_PAM "enable pam authentication" ON)
if ("${WEBSERVER_ENABLE_PAM}")
    find_package (PAM REQUIRED)
else ()
    add_definitions ("-DWEBSERVER_DISABLE_PAM")
endif ()

add_definitions ("-Wno-attributes")
# Copy pam-webserver to etc/pam.d
install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/pam-webserver DESTINATION /etc/pam.d/
         RENAME webserver)

# tinyxml2
find_package (tinyxml2 REQUIRED)

set (WEBSERVER_MAIN src/webserver_main.cpp)

include_directories (${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories (${CMAKE_CURRENT_SOURCE_DIR}/redfish-core/include)
include_directories (${CMAKE_CURRENT_SOURCE_DIR}/redfish-core/lib)

file (MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/include/bmcweb)
include_directories (${CMAKE_BINARY_DIR}/include)

set (SRC_FILES redfish-core/src/error_messages.cpp
     redfish-core/src/utils/json_utils.cpp ${GENERATED_SRC_FILES})

file (COPY src/test_resources DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Unit Tests
if (NOT ${YOCTO_DEPENDENCIES})
    set (UT_FILES src/gtest_main.cpp src/msan_test.cpp
         redfish-core/ut/privileges_test.cpp
         redfish-core/ut/lock_test.cpp
         ${CMAKE_BINARY_DIR}/include/bmcweb/blns.hpp) # big list of naughty
                                                      # strings
    add_custom_command (OUTPUT ${CMAKE_BINARY_DIR}/include/bmcweb/blns.hpp
                        COMMAND
                            xxd -i
                            ${CMAKE_CURRENT_SOURCE_DIR}/src/test_resources/blns
                            ${CMAKE_BINARY_DIR}/include/bmcweb/blns.hpp)

    set_source_files_properties (${CMAKE_BINARY_DIR}/include/bmcweb/blns.hpp
                                 PROPERTIES GENERATED TRUE)
    enable_testing ()

    add_executable (webtest ${SRC_FILES} ${UT_FILES})

    find_package (GTest REQUIRED)
    find_package (GMock REQUIRED)
    target_link_libraries (webtest ${GTEST_LIBRARIES})
    target_link_libraries (webtest ${GMOCK_LIBRARIES})

    target_link_libraries (webtest pthread)
    target_link_libraries (webtest ${OPENSSL_LIBRARIES})
    target_link_libraries (webtest ${ZLIB_LIBRARIES})
    target_link_libraries (webtest pam)
    target_link_libraries (webtest tinyxml2)
    target_link_libraries (webtest sdbusplus)
    target_link_libraries (webtest -lsystemd)
    target_link_libraries (webtest -lstdc++fs)
    add_test (webtest webtest "--gtest_output=xml:webtest.xml")

endif (NOT ${YOCTO_DEPENDENCIES})

install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/static/ DESTINATION share/www)

# bmcweb
add_executable (bmcweb ${WEBSERVER_MAIN} ${HDR_FILES} ${SRC_FILES})
target_link_libraries (bmcweb ${OPENSSL_LIBRARIES})
target_link_libraries (bmcweb ${ZLIB_LIBRARIES})
target_link_libraries (bmcweb pam)
target_link_libraries (bmcweb -latomic)
target_link_libraries (bmcweb -lsystemd)
target_link_libraries (bmcweb -lstdc++fs)
target_link_libraries (bmcweb sdbusplus)
target_link_libraries (bmcweb tinyxml2)
install (TARGETS bmcweb DESTINATION bin)

target_compile_definitions (
    bmcweb PRIVATE $<$<BOOL:${BMCWEB_ENABLE_KVM}>: -DBMCWEB_ENABLE_KVM>
    $<$<BOOL:${BMCWEB_ENABLE_MUTUAL_TLS_AUTHENTICATION}>: -DBMCWEB_ENABLE_MUTUAL_TLS_AUTHENTICATION>
    $<$<BOOL:${BMCWEB_ENABLE_VM_WEBSOCKET}>: -DBMCWEB_ENABLE_VM_WEBSOCKET>
    $<$<BOOL:${BMCWEB_ENABLE_VM_NBDPROXY}>: -DBMCWEB_ENABLE_VM_NBDPROXY>
    $<$<BOOL:${BMCWEB_ENABLE_DBUS_REST}>: -DBMCWEB_ENABLE_DBUS_REST>
    $<$<BOOL:${BMCWEB_ENABLE_REDFISH}>: -DBMCWEB_ENABLE_REDFISH>
    $<$<BOOL:${BMCWEB_ENABLE_STATIC_HOSTING}>: -DBMCWEB_ENABLE_STATIC_HOSTING>
    $<$<BOOL:${BMCWEB_ENABLE_HOST_SERIAL_WEBSOCKET}>:
    -DBMCWEB_ENABLE_HOST_SERIAL_WEBSOCKET>
    $<$<BOOL:${BMCWEB_INSECURE_DISABLE_CSRF_PREVENTION}>:
    -DBMCWEB_INSECURE_DISABLE_CSRF_PREVENTION>
    $<$<BOOL:${BMCWEB_INSECURE_DISABLE_SSL}>: -DBMCWEB_INSECURE_DISABLE_SSL>
    $<$<BOOL:${BMCWEB_INSECURE_DISABLE_XSS_PREVENTION}>:
    -DBMCWEB_INSECURE_DISABLE_XSS_PREVENTION>
    $<$<BOOL:${BMCWEB_ENABLE_REDFISH_RAW_PECI}>:
    -DBMCWEB_ENABLE_REDFISH_RAW_PECI>
    $<$<BOOL:${BMCWEB_ENABLE_REDFISH_CPU_LOG}>:
    -DBMCWEB_ENABLE_REDFISH_CPU_LOG>
    $<$<BOOL:${BMCWEB_ENABLE_REDFISH_DUMP_LOG}>:
    -DBMCWEB_ENABLE_REDFISH_DUMP_LOG>
    $<$<BOOL:${BMCWEB_ENABLE_REDFISH_BMC_JOURNAL}>:
    -DBMCWEB_ENABLE_REDFISH_BMC_JOURNAL>
    $<$<BOOL:${BMCWEB_ENABLE_REDFISH_DBUS_LOG_ENTRIES}>:
    -DBMCWEB_ENABLE_REDFISH_DBUS_LOG_ENTRIES>
    $<$<BOOL:${BMCWEB_INSECURE_ENABLE_REDFISH_FW_TFTP_UPDATE}>:
    -DBMCWEB_INSECURE_ENABLE_REDFISH_FW_TFTP_UPDATE>
    $<$<BOOL:${BMCWEB_ENABLE_REDFISH_PROVISIONING_FEATURE}>:
    -DBMCWEB_ENABLE_REDFISH_PROVISIONING_FEATURE>
    $<$<BOOL:${BMCWEB_ENABLE_VALIDATION_UNSECURE_FEATURE}>:
    -DBMCWEB_ENABLE_VALIDATION_UNSECURE_FEATURE>
    $<$<BOOL:${BMCWEB_INSECURE_UNRESTRICTED_SENSOR_OVERRIDE}>:
    -DBMCWEB_INSECURE_UNRESTRICTED_SENSOR_OVERRIDE>
    $<$<BOOL:${BMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE}>:
    -DBMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE>
    $<$<BOOL:${BMCWEB_INSECURE_ENABLE_HTTP_PUSH_STYLE_EVENTING}>:
    -DBMCWEB_INSECURE_ENABLE_HTTP_PUSH_STYLE_EVENTING>
)

# configure and install systemd unit files
configure_file (bmcweb.socket bmcweb.socket COPYONLY)
configure_file (bmcweb.service.in bmcweb.service)
pkg_get_variable (SYSTEMD_SYSTEMUNITDIR systemd systemdsystemunitdir)
install (FILES ${PROJECT_BINARY_DIR}/bmcweb.socket DESTINATION
               ${SYSTEMD_SYSTEMUNITDIR})
install (FILES ${PROJECT_BINARY_DIR}/bmcweb.service DESTINATION
               ${SYSTEMD_SYSTEMUNITDIR})
