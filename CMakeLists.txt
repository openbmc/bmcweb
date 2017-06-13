cmake_minimum_required(VERSION 2.8.10 FATAL_ERROR)

cmake_policy(SET CMP0054 NEW)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

message("${CMAKE_MODULE_PATH}")

SET(BUILD_SHARED_LIBRARIES OFF)

#SET(HUNTER_STATUS_DEBUG ON)
option(HUNTER_ENABLED "Enable hunter package pulling" OFF)
#SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fno-rtti")
#SET(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -fno-rtti")
include("cmake/HunterGate.cmake")
HunterGate(
    URL "https://github.com/ruslo/hunter/archive/v0.18.64.tar.gz"
    SHA1 "baf9c8cc4f65306f0e442b5419967b4c4c04589a"
)

project(bmc-webserver CXX C)

include( CTest )

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (MSAN)
  if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "Cannot enable MSAN unless using Clang")
  endif()

  if (ASAN)
    message(FATAL_ERROR "ASAN and MSAN are mutually exclusive")
  endif()

  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MSAN_CXX_FLAGS}")
  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${MSAN_LINKER_EXE_FLAGS}")

  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=memory -fsanitize-memory-track-origins -fno-omit-frame-pointer")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=memory -fsanitize-memory-track-origins -fno-omit-frame-pointer")
  set(OPENSSL_NO_ASM "1")
  set(LIBC++ ON)
endif(MSAN)

if (ASAN)
  if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "Cannot enable ASAN unless using Clang")
  endif()

  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fsanitize-address-use-after-scope -fno-omit-frame-pointer")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize-address-use-after-scope -fno-omit-frame-pointer")
  set(OPENSSL_NO_ASM "1")
endif(ASAN)

if (GCOV)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
endif(GCOV)

if ( ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR} )
    message( FATAL_ERROR "In-source builds not allowed. Please make a new directory (usually called build) and run CMake from there. You may need to remove CMakeCache.txt." )
endif()

if(LIBC++)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lc++abi")
  if (MSAN)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I/home/ed/libcxx_msan/include -I/home/ed/libcxx_msan/include/c++/v1")
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L/home/ed/libcxx_msan/lib -Wl,-rpath,I/home/ed/libcxx_msan/lib")
  endif(MSAN)
endif(LIBC++)

#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall")

# general
option(BUILD_SHARED_LIBS "Build as shared library" OFF)
option(BUILD_UT "Enable Unit test" ON)

# This needs to be before the crow and other module includes so headers get overriden correctly
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# security flags
#SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fstack-protector-strong -fPIE -fPIC -D_FORTIFY_SOURCE=2 -Wformat -Wformat-security" )
#SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -z noexecstack -z relro -z now")

# Boost
#add_definitions(-DBOOST_NO_RTTI -DBOOST_NO_TYPEID)

#add_definitions(-DBOOST_ASIO_ENABLE_HANDLER_TRACKING)
add_definitions(-DBOOST_ERROR_CODE_HEADER_ONLY)
add_definitions(-DBOOST_SYSTEM_NO_DEPRECATED)
add_definitions(-DBOOST_ALL_NO_LIB)
#set(Boost_USE_STATIC_LIBS ON)
hunter_add_package(Boost)
find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS}) 

#Openssl
hunter_add_package(OpenSSL)
find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})
message("OPENSSL_INCLUDE_DIR ${OPENSSL_INCLUDE_DIR}")

#g3 logging
# G3logger does some unfortunate compile options, so cheat a little bit and copy/paste
set(LOG_SRC ${CMAKE_CURRENT_SOURCE_DIR}/g3log)

file(GLOB_RECURSE SRC_FILES ${LOG_SRC}/*.cpp ${LOG_SRC}/*.ipp)
file(GLOB_RECURSE HEADER_FILES ${LOG_SRC}/*.hpp)

IF (MSVC OR MINGW)
    list(REMOVE_ITEM SRC_FILES  ${LOG_SRC}/crashhandler_unix.cpp)
ELSE()
    list(REMOVE_ITEM SRC_FILES  ${LOG_SRC}/crashhandler_windows.cpp ${LOG_SRC}/g3log/stacktrace_windows.hpp ${LOG_SRC}/stacktrace_windows.cpp)
ENDIF (MSVC OR MINGW)

# Create the g3log library
include_directories(${LOG_SRC})

add_library(g3logger ${SRC_FILES})
set_target_properties(g3logger PROPERTIES LINKER_LANGUAGE CXX)
# clean up some warnings in files we don't own
if(("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU") OR ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"))
    set_source_files_properties(g3log/src/logcapture.cpp PROPERTIES COMPILE_FLAGS -Wno-braced-scalar-init)
    set_source_files_properties(g3log/src/filesink.cpp PROPERTIES COMPILE_FLAGS -Wno-braced-scalar-init)
    set_source_files_properties(g3log/src/logworker.cpp PROPERTIES COMPILE_FLAGS -Wno-braced-scalar-init)
endif()

#lib jpeg
set(BUILD_STATIC ON)
#include_directories(libjpeg)
#include_directories(${CMAKE_CURRENT_BINARY_DIR}/libjpeg)
#add_subdirectory(libjpeg)


# dbus
hunter_add_package(dbus)
find_package(dbus REQUIRED) # Include functions provided by PkgConfig module.
include_directories(${DBUS_INCLUDE_DIRS})

# Crow
#add_definitions(-DCROW_DISABLE_LOGGING)
add_definitions(-DCROW_ENABLE_SSL)
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/crow/include)

#Zlib
hunter_add_package(ZLIB)
find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})


# PAM
find_package(PAM REQUIRED)

# Boost-dbus
if(${BUILD_UT})
    add_subdirectory(boost-dbus)
endif()
include_directories(boost-dbus/include)

set(WEBSERVER_MAIN src/webserver_main.cpp)

set(HDR_FILES
    include/crow/g3_logger.hpp
    include/ssl_key_handler.hpp
    include/color_cout_g3_sink.hpp
)

set(GENERATED_SRC_FILES 
    ${CMAKE_BINARY_DIR}/generated/webassets.cpp
    ${CMAKE_BINARY_DIR}/generated/webassets.hpp
)

include_directories(${CMAKE_BINARY_DIR}/generated)

set_source_files_properties(${GENERATED_SRC_FILES} PROPERTIES GENERATED TRUE)

# the webassets file has a non-ideal beahvior, where it loads binary data into a std::string
# due to the way crow is constructed.  This causes GCC warnings.  Filter them out.
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.9)
        set_source_files_properties(${CMAKE_BINARY_DIR}/generated/webassets.cpp PROPERTIES COMPILE_FLAGS -Wno-narrowing)
    endif(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.9)
endif()


set(SRC_FILES
    src/base64.cpp
    ${GENERATED_SRC_FILES}
)

set(UT_FILES
    src/crow_test.cpp
    src/gtest_main.cpp
    src/base64_test.cpp
    src/token_authorization_middleware_test.cpp
    src/security_headers_middleware_test.cpp
    src/webassets_test.cpp
    src/crow_getroutes_test.cpp
    src/ast_jpeg_decoder_test.cpp
    src/kvm_websocket_test.cpp
    src/test_utils.cpp
    src/msan_test.cpp
    src/ci_map_tests.cpp
    src/ast_video_puller_test.cpp
    ${CMAKE_BINARY_DIR}/generated/blns.hpp
)


file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/generated")

file(COPY src/test_resources DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Unit Tests
if(${BUILD_UT})
    # big list of naughty strings
    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/generated/blns.hpp
        COMMAND xxd -i ${CMAKE_CURRENT_SOURCE_DIR}/src/test_resources/blns ${CMAKE_BINARY_DIR}/generated/blns.hpp)

    # googletest
    enable_testing()

    #hunter_add_package(GTest)
    add_subdirectory(googletest)
    #find_package(GMock CONFIG REQUIRED)

    add_executable(webtest ${HDR_FILES} ${SRC_FILES} ${UT_FILES})
    target_link_libraries(webtest gmock gtest)
    target_link_libraries(webtest pthread)
    target_link_libraries(webtest g3logger)
    
    target_link_libraries(webtest ${OPENSSL_LIBRARIES})
    target_link_libraries(webtest ${ZLIB_LIBRARIES})
    target_link_libraries(webtest pam)
    add_dependencies(webtest packagestaticcpp)
    add_test(webtest webtest "--gtest_output=xml:webtest.xml")

endif(${BUILD_UT})

# web static assets
add_subdirectory(static)


# bmcweb
add_executable(bmcweb ${WEBSERVER_MAIN} ${HDR_FILES} ${SRC_FILES})
target_link_libraries(bmcweb pthread)
target_link_libraries(bmcweb g3logger)
target_link_libraries(bmcweb ${OPENSSL_LIBRARIES})
target_link_libraries(bmcweb ${ZLIB_LIBRARIES})
target_link_libraries(bmcweb  ${DBUS_LIBRARIES})
target_link_libraries(bmcweb pam)
add_dependencies(bmcweb packagestaticcpp)
install (TARGETS bmcweb DESTINATION bin)

add_executable(getvideo src/getvideo_main.cpp)
target_link_libraries(getvideo pthread)
target_link_libraries(getvideo g3logger)

# Visual Studio Code helper
# this needs to be at the end to make sure all includes are handled correctly

get_property(C_INCLUDE_DIRS DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
execute_process(COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/scripts/prime_vscode_compile_db.py ${C_INCLUDE_DIRS})
