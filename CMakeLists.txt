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
hunter_add_package(OpenSSL)
find_package(OpenSSL REQUIRED)
if (NOT OPENSSL_FOUND)
    message(FATAL_ERROR "Could not find OpenSSL")
endif()
include_directories(${OPENSSL_INCLUDE_DIR})

# Crow
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/crow/include)

#g3 logging
set(ADD_FATAL_EXAMPLE OFF)
add_subdirectory(g3log)
include_directories(g3log/src)

# Debug sanitizers
find_package(Sanitizers)

# Executable
add_executable(example ${CMAKE_CURRENT_SOURCE_DIR}/src/example.cpp)
#target_link_libraries(example crow)
target_link_libraries(example Boost::boost Boost::system)
target_link_libraries(example ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(example OpenSSL::SSL OpenSSL::Crypto)
target_link_libraries(example g3logger)


