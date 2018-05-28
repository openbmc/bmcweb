cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

cmake_policy(SET CMP0054 NEW)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

option(BUILD_STATIC_LIBS "Built static libraries" ON)
option(YOCTO_DEPENDENCIES "Use YOCTO depedencies system" OFF)

project(bmc-webserver CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -Wall")

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-rtti")

# general
option(BMCWEB_BUILD_UT "Enable Unit test" ON)

# security flags
SET(
  SECURITY_FLAGS
  "\
    -fstack-protector-strong \
    -fPIE \
    -fPIC \
    -D_FORTIFY_SOURCE=2 \
    -Wformat \
    -Wformat-security"
  )
SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${SECURITY_FLAGS}")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO
    "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${SECURITY_FLAGS}")
SET(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} ${SECURITY_FLAGS}")

# Enable link time optimization This is a temporary workaround because
# INTERPROCEDURAL_OPTIMIZATION isn't available until cmake 3.9. gcc-ar and gcc-
# ranlib are wrappers around ar and ranlib which add the lto plugin to the
# command line.
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  if(NOT CMAKE_BUILD_TYPE MATCHES Debug)
    STRING(REGEX REPLACE "ar$" "gcc-ar" CMAKE_AR ${CMAKE_AR})
    STRING(REGEX REPLACE "ranlib$" "gcc-ranlib" CMAKE_RANLIB ${CMAKE_RANLIB})
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto -fno-fat-lto-objects")

    # Reduce the binary size by removing unnecessary dynamic symbol table
    # entries
    SET(
      CMAKE_CXX_FLAGS
      "${CMAKE_CXX_FLAGS} \
        -fvisibility=hidden \
        -fvisibility-inlines-hidden \
        -Wl,--exclude-libs,ALL"
      )
  endif(NOT CMAKE_BUILD_TYPE MATCHES Debug)
endif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")

if(NOT ${YOCTO_DEPENDENCIES}) # Download and unpack googletest at configure time
  configure_file(CMakeLists.txt.in 3rdparty/CMakeLists.txt)
  execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
                  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/3rdparty)
  execute_process(COMMAND ${CMAKE_COMMAND} --build .
                  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/3rdparty)

  set(CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR}/prefix ${CMAKE_PREFIX_PATH})
endif()

# add_definitions(-DBOOST_ASIO_ENABLE_HANDLER_TRACKING)
add_definitions(-DBOOST_ERROR_CODE_HEADER_ONLY)
add_definitions(-DBOOST_SYSTEM_NO_DEPRECATED)
add_definitions(-DBOOST_ALL_NO_LIB)
add_definitions(-DBOOST_NO_RTTI)
add_definitions(-DBOOST_NO_TYPEID)
# set(Boost_USE_STATIC_LIBS ON)

find_package(Boost 1.64 REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

# Openssl
find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})
message("OPENSSL_INCLUDE_DIR ${OPENSSL_INCLUDE_DIR}")

# Crow
message("CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
if(CMAKE_BUILD_TYPE MATCHES Debug)
  message("Logging disabled")
  add_definitions(-DCROW_ENABLE_LOGGING)
  add_definitions(-DCROW_ENABLE_DEBUG)
endif(CMAKE_BUILD_TYPE MATCHES Debug)

add_definitions(-DCROW_ENABLE_SSL)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/crow/include)

# Zlib
find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})

# PAM
option(WEBSERVER_ENABLE_PAM "enable pam authentication" ON)
if("${WEBSERVER_ENABLE_PAM}")
  find_package(PAM REQUIRED)
else()
  add_definitions("-DWEBSERVER_DISABLE_PAM")
endif()

add_definitions("-Wno-attributes")

# TODO Enable OCP custom flag
add_definitions(-DOCP_CUSTOM_FLAG)

# Boost-dbus
find_package(boost-dbus REQUIRED)

# tinyxml2
find_package(tinyxml2 REQUIRED)

set(WEBSERVER_MAIN src/webserver_main.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/redfish-core/include)

set(SRC_FILES
    redfish-core/src/error_messages.cpp
    redfish-core/src/utils/json_utils.cpp
    ${GENERATED_SRC_FILES}
)

file(COPY src/test_resources DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/generated)

# Unit Tests
if(${BMCWEB_BUILD_UT})
  set(UT_FILES
      src/crow_test.cpp
      src/gtest_main.cpp
      src/token_authorization_middleware_test.cpp
      src/security_headers_middleware_test.cpp
      src/webassets_test.cpp
      src/crow_getroutes_test.cpp
      src/ast_jpeg_decoder_test.cpp
      src/kvm_websocket_test.cpp
      src/msan_test.cpp
      src/ci_map_tests.cpp
      src/ast_video_puller_test.cpp
      ${CMAKE_BINARY_DIR}/generated/blns.hpp) # big list of naughty strings
  add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/generated/blns.hpp
                     COMMAND
                       xxd -i
                       ${CMAKE_CURRENT_SOURCE_DIR}/src/test_resources/blns
                       ${CMAKE_BINARY_DIR}/generated/blns.hpp)

  set_source_files_properties(${CMAKE_BINARY_DIR}/generated/blns.hpp
                              PROPERTIES
                              GENERATED
                              TRUE) # googletest
  enable_testing()

  add_executable(webtest ${SRC_FILES} ${UT_FILES})

  find_package(GTest REQUIRED)
  find_package(GMock REQUIRED)
  target_link_libraries(webtest ${GTEST_LIBRARIES})
  target_link_libraries(webtest ${GMOCK_LIBRARIES})

  target_link_libraries(webtest pthread)
  target_link_libraries(webtest boost-dbus)
  target_link_libraries(webtest ${OPENSSL_LIBRARIES})
  target_link_libraries(webtest ${ZLIB_LIBRARIES})
  target_link_libraries(webtest pam)
  target_link_libraries(webtest tinyxml2)
  target_link_libraries(webtest -lstdc++fs)
  add_test(webtest webtest "--gtest_output=xml:webtest.xml")

endif(${BMCWEB_BUILD_UT})

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/static/ DESTINATION share/www)

# bmcweb
add_executable(bmcweb ${WEBSERVER_MAIN} ${HDR_FILES} ${SRC_FILES})
target_link_libraries(bmcweb boost-dbus)
target_link_libraries(bmcweb pthread)
target_link_libraries(bmcweb ${OPENSSL_LIBRARIES})
target_link_libraries(bmcweb ${ZLIB_LIBRARIES})
target_link_libraries(bmcweb pam)
target_link_libraries(bmcweb -lsystemd)
target_link_libraries(bmcweb -lstdc++fs)
target_link_libraries(bmcweb tinyxml2)
install(TARGETS bmcweb DESTINATION bin)

add_executable(getvideo src/getvideo_main.cpp)
target_link_libraries(getvideo pthread)

# Visual Studio Code helper this needs to be at the end to make sure all
# includes are handled correctly
include(CMakeExtraGeneratorDetermineCompilerMacrosAndIncludeDirs)
get_property(C_INCLUDE_DIRS
             DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
             PROPERTY INCLUDE_DIRECTORIES)

execute_process(COMMAND
                  python3
                  ${CMAKE_CURRENT_SOURCE_DIR}/scripts/prime_vscode_compile_db.py
                  ${C_INCLUDE_DIRS}
                  ${CMAKE_EXTRA_GENERATOR_C_SYSTEM_INCLUDE_DIRS}
                  ${CMAKE_EXTRA_GENERATOR_CXX_SYSTEM_INCLUDE_DIRS})
