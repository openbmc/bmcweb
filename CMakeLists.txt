cmake_minimum_required (VERSION 3.5 FATAL_ERROR)

cmake_policy (SET CMP0054 NEW)

set (CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

option (BUILD_STATIC_LIBS "Built static libraries" ON)

option (YOCTO_DEPENDENCIES "Use YOCTO dependencies system" OFF)

option (BMCWEB_ENABLE_KVM "Enable the KVM host video WebSocket.  Path is
       '/kvmws'.  Video is from the BMC's '/dev/video' device." ON)
option (BMCWEB_ENABLE_DBUS_REST "Enable Phosphor REST (D-Bus) APIs.  Paths
       directly map Phosphor D-Bus object paths, for example,
       '/xyz/openbmc_project/logging/entry/enumerate'.  See
       https://github.com/openbmc/docs/blob/master/rest-api.md." ON)
option (BMCWEB_ENABLE_REDFISH "Enable Redfish APIs.  Paths are under
       '/redfish/v1/'.  See
       https://github.com/openbmc/bmcweb/blob/master/DEVELOPING.md#redfish."
       ON)
option (BMCWEB_ENABLE_HOST_SERIAL_WEBSOCKET "Enable host serial console
       WebSocket.  Path is '/console0'.  See
       https://github.com/openbmc/docs/blob/master/console.md." ON)
option (BMCWEB_ENABLE_STATIC_HOSTING "Enable serving files from the
       '/usr/share/www' directory as paths under '/'." ON)
option (BMCWEB_ENABLE_REDFISH_BMC_JOURNAL "Enable BMC journal access through
       Redfish.  Paths are under
       '/redfish/v1/Managers/bmc/LogServices/Journal'." OFF)
option (BMCWEB_ENABLE_REDFISH_RAW_PECI "Enable PECI transactions through
       Redfish.  Paths are under '/redfish/v1/Systems/system/LogServices/CpuLog/
       Actions/Oem/CpuLog.SendRawPeci'." OFF)
option (BMCWEB_ENABLE_REDFISH_CPU_LOG "Enable CPU log service transactions
       through Redfish.  Paths are under
       '/redfish/v1/Systems/system/LogServices/CpuLog'." OFF)

# Insecure options.  Every option that starts with a BMCWEB_INSECURE flag should
# not be enabled by default for any platform, unless the author fully
# comprehends the implications of doing so.  In general, enabling these options
# will cause security problems of varying degrees
option (BMCWEB_INSECURE_DISABLE_CSRF_PREVENTION "Disable CSRF prevention checks.
       Should be set to OFF for production systems." OFF)

option (BMCWEB_INSECURE_DISABLE_SSL "Disable SSL ports. Should be set to OFF for
       production systems." OFF)

option (BMCWEB_INSECURE_DISABLE_AUTHENTICATION "Disable authentication on all
       ports. Should be set to OFF for production systems" OFF)

option (BMCWEB_INSECURE_DISABLE_XSS_PREVENTION "Disable XSS preventions" OFF)

project (bmc-webserver CXX)

include (CTest)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

set (CMAKE_EXPORT_COMPILE_COMMANDS ON)

set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -Wall")

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-rtti")

# general
option (BMCWEB_BUILD_UT "Enable Unit test" OFF)

# security flags
set (SECURITY_FLAGS "\
    -fstack-protector-strong \
    -fPIE \
    -fPIC \
    -D_FORTIFY_SOURCE=2 \
    -Wformat \
    -Wformat-security")
set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${SECURITY_FLAGS}")
set (
    CMAKE_CXX_FLAGS_RELWITHDEBINFO
    "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${SECURITY_FLAGS}"
)
set (CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} ${SECURITY_FLAGS}")

# Enable link time optimization This is a temporary workaround because
# INTERPROCEDURAL_OPTIMIZATION isn't available until cmake 3.9. gcc-ar and gcc-
# ranlib are wrappers around ar and ranlib which add the lto plugin to the
# command line.
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if (NOT CMAKE_BUILD_TYPE MATCHES Debug)
        string (REGEX REPLACE "ar$" "gcc-ar" CMAKE_AR ${CMAKE_AR})
        string (
            REGEX
            REPLACE "ranlib$" "gcc-ranlib" CMAKE_RANLIB ${CMAKE_RANLIB}
        )
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto -fno-fat-lto-objects")

        # Reduce the binary size by removing unnecessary dynamic symbol table
        # entries
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
        -fvisibility=hidden \
        -fvisibility-inlines-hidden \
        -Wl,--exclude-libs,ALL")
    endif (NOT CMAKE_BUILD_TYPE MATCHES Debug)
endif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")

if (NOT ${YOCTO_DEPENDENCIES}) # Download and unpack googletest at configure
                               # time
    configure_file (CMakeLists.txt.in 3rdparty/CMakeLists.txt)
    execute_process (
        COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/3rdparty
    )
    execute_process (
        COMMAND ${CMAKE_COMMAND} --build .
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/3rdparty
    )

    set (CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR}/prefix ${CMAKE_PREFIX_PATH})
endif ()

# add_definitions(-DBOOST_ASIO_ENABLE_HANDLER_TRACKING)
add_definitions (-DBOOST_ASIO_DISABLE_THREADS)
add_definitions (-DBOOST_ERROR_CODE_HEADER_ONLY)
add_definitions (-DBOOST_SYSTEM_NO_DEPRECATED)
add_definitions (-DBOOST_ALL_NO_LIB)
add_definitions (-DBOOST_NO_RTTI)
add_definitions (-DBOOST_NO_TYPEID)
add_definitions (-DBOOST_COROUTINES_NO_DEPRECATION_WARNING)

find_package (Boost 1.66 REQUIRED)
include_directories (${BOOST_SRC_DIR})

# sdbusplus
if (NOT ${YOCTO_DEPENDENCIES})
    include_directories (${CMAKE_BINARY_DIR}/sdbusplus-src)
    link_directories (${CMAKE_BINARY_DIR}/sdbusplus-src/.libs)
endif ()

# Openssl
find_package (OpenSSL REQUIRED)
include_directories (${OPENSSL_INCLUDE_DIR})
message ("OPENSSL_INCLUDE_DIR ${OPENSSL_INCLUDE_DIR}")

# bmcweb
message ("CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
if (CMAKE_BUILD_TYPE MATCHES Debug)
    message ("Logging disabled")
    add_definitions (-DBMCWEB_ENABLE_LOGGING)
    add_definitions (-DBMCWEB_ENABLE_DEBUG)
endif (CMAKE_BUILD_TYPE MATCHES Debug)

if (NOT "${BMCWEB_INSECURE_DISABLE_SSL}")
    add_definitions (-DBMCWEB_ENABLE_SSL)
endif (NOT "${BMCWEB_INSECURE_DISABLE_SSL}")
include_directories (${CMAKE_CURRENT_SOURCE_DIR}/crow/include)

# Zlib
find_package (ZLIB REQUIRED)
include_directories (${ZLIB_INCLUDE_DIRS})

# PAM
option (WEBSERVER_ENABLE_PAM "enable pam authentication" ON)
if ("${WEBSERVER_ENABLE_PAM}")
    find_package (PAM REQUIRED)
else ()
    add_definitions ("-DWEBSERVER_DISABLE_PAM")
endif ()

add_definitions ("-Wno-attributes")
# Copy pam-webserver to etc/pam.d
install (
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/pam-webserver
    DESTINATION /etc/pam.d/
    RENAME webserver
)

# tinyxml2
find_package (tinyxml2 REQUIRED)

set (WEBSERVER_MAIN src/webserver_main.cpp)

include_directories (${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories (${CMAKE_CURRENT_SOURCE_DIR}/redfish-core/include)

file (MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/include/bmcweb)
include_directories (${CMAKE_BINARY_DIR}/include)

set (
    SRC_FILES redfish-core/src/error_messages.cpp
    redfish-core/src/utils/json_utils.cpp ${GENERATED_SRC_FILES}
)

file (COPY src/test_resources DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Unit Tests
if (${BMCWEB_BUILD_UT})
    set (
        UT_FILES src/crow_test.cpp src/gtest_main.cpp
        src/token_authorization_middleware_test.cpp
        src/security_headers_middleware_test.cpp src/webassets_test.cpp
        src/crow_getroutes_test.cpp src/ast_jpeg_decoder_test.cpp
        src/kvm_websocket_test.cpp src/msan_test.cpp
        src/ast_video_puller_test.cpp src/openbmc_jtag_rest_test.cpp
        redfish-core/ut/privileges_test.cpp
        ${CMAKE_BINARY_DIR}/include/bmcweb/blns.hpp
    ) # big list of naughty strings
    add_custom_command (
        OUTPUT ${CMAKE_BINARY_DIR}/include/bmcweb/blns.hpp
        COMMAND
            xxd -i ${CMAKE_CURRENT_SOURCE_DIR}/src/test_resources/blns
            ${CMAKE_BINARY_DIR}/include/bmcweb/blns.hpp
    )

    set_source_files_properties (
        ${CMAKE_BINARY_DIR}/include/bmcweb/blns.hpp PROPERTIES GENERATED TRUE
    )
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

endif (${BMCWEB_BUILD_UT})

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

add_executable (getvideo src/getvideo_main.cpp)
target_link_libraries (getvideo pthread)

target_compile_definitions (
    bmcweb PRIVATE
    $<$<BOOL:${BMCWEB_ENABLE_KVM}>: -DBMCWEB_ENABLE_KVM>
    $<$<BOOL:${BMCWEB_ENABLE_DBUS_REST}>: -DBMCWEB_ENABLE_DBUS_REST>
    $<$<BOOL:${BMCWEB_ENABLE_REDFISH}>: -DBMCWEB_ENABLE_REDFISH>
    $<$<BOOL:${BMCWEB_ENABLE_STATIC_HOSTING}>: -DBMCWEB_ENABLE_STATIC_HOSTING>
    $<$<BOOL:${BMCWEB_ENABLE_HOST_SERIAL_WEBSOCKET}>: -DBMCWEB_ENABLE_HOST_SERIAL_WEBSOCKET>
    $<$<BOOL:${BMCWEB_INSECURE_DISABLE_CSRF_PREVENTION}>: -DBMCWEB_INSECURE_DISABLE_CSRF_PREVENTION>
    $<$<BOOL:${BMCWEB_INSECURE_DISABLE_SSL}>: -DBMCWEB_INSECURE_DISABLE_SSL>
    $<$<BOOL:${BMCWEB_INSECURE_DISABLE_XSS_PREVENTION}>: -DBMCWEB_INSECURE_DISABLE_XSS_PREVENTION>
    $<$<BOOL:${BMCWEB_ENABLE_REDFISH_RAW_PECI}>: -DBMCWEB_ENABLE_REDFISH_RAW_PECI>
    $<$<BOOL:${BMCWEB_ENABLE_REDFISH_CPU_LOG}>: -DBMCWEB_ENABLE_REDFISH_CPU_LOG>
    $<$<BOOL:${BMCWEB_ENABLE_REDFISH_BMC_JOURNAL}>: -DBMCWEB_ENABLE_REDFISH_BMC_JOURNAL>
)
