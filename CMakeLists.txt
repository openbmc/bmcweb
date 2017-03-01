cmake_minimum_required(VERSION 2.8.10 FATAL_ERROR)

cmake_policy(SET CMP0054 OLD)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

message("${CMAKE_MODULE_PATH}")

#set(HUNTER_ROOT /home/ed/hunter)
SET(HUNTER_STATUS_DEBUG ON)
include("cmake/HunterGate.cmake")

HunterGate(
    URL "https://github.com/ruslo/hunter/archive/v0.16.31.tar.gz"
    SHA1 "8fcc0a2d6206e1f2c6fc011e3e694e388d030b53"
)

option(BUILD_FOR_EMBEDDED "Build for device target" ON)

project(bmc-webserver CXX C)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if ( ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR} )
    message( FATAL_ERROR "In-source builds not allowed. Please make a new directory (usually called build) and run CMake from there. You may need to remove CMakeCache.txt." )
endif()

# general
option(BUILD_SHARED_LIBS "Build Pion as shared library" OFF)
option(BUILD_UT "Enable Unit test" OFF)

# Boost
add_definitions(-DBOOST_ALL_NO_LIB)
set(Boost_USE_STATIC_LIBS ON)
hunter_add_package(Boost COMPONENTS system thread regex)
find_package(Boost COMPONENTS system thread regex REQUIRED)

#Openssl
#hunter_add_package(OpenSSL)
find_package(OpenSSL REQUIRED)
if (NOT OPENSSL_FOUND)
    message(FATAL_ERROR "Could not find OpenSSL")
endif()
include_directories(${OPENSSL_INCLUDE_DIR})

# Crow
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/crow/include)

#g3 logging
option(ADD_FATAL_EXAMPLE "Disable g3 examples" OFF)
add_subdirectory(g3log)
include_directories(g3log/src)

# Debug sanitizers
find_package(Sanitizers)

# C++ GSL (Guideline support libraries)
include_directories(GSL/include)

set(WEBSERVER_MAIN src/webserver_main.cpp)

set(SRC_FILES
    include/crow_g3_logger.hpp
    include/ssl_key_handler.hpp
    include/color_cout_g3_sink.hpp
    src/token_authorization_middleware.cpp
    src/base64.cpp
)

set(UT_FILES
    src/gtest_main.cpp
    src/base64_test.cpp
    src/token_authorization_middleware_test.cpp
    ${CMAKE_BINARY_DIR}/generated/blns.hpp
)

# big list of naughty strings
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/generated")
add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/generated/blns.hpp
    COMMAND xxd -i ${CMAKE_CURRENT_SOURCE_DIR}/src/blns.txt ${CMAKE_BINARY_DIR}/generated/blns.hpp)

# googletest
#find_package(GTest REQUIRED)
enable_testing()
find_package(GTest REQUIRED)

add_executable(unittest ${SRC_FILES} ${UT_FILES})
target_link_libraries(unittest GTest::GTest GTest::Main)
target_link_libraries(unittest Boost::boost Boost::system)
target_link_libraries(unittest ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(unittest OpenSSL::SSL OpenSSL::Crypto)
target_link_libraries(unittest g3logger)

# bmcweb
add_executable(bmcweb ${WEBSERVER_MAIN} ${SRC_FILES})
target_link_libraries(bmcweb Boost::boost Boost::system)
target_link_libraries(bmcweb ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(bmcweb OpenSSL::SSL OpenSSL::Crypto)
target_link_libraries(bmcweb g3logger)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# this needs to be at the end to make sure all includes are handled correctly
get_property(C_INCLUDE_DIRS DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
execute_process(COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/scripts/prime_vscode_compile_db.py ${C_INCLUDE_DIRS})
