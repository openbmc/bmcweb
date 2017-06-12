# Copyright (c) Benjamin Kietzman (github.com/bkietz)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

project(boost-dbus CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

###############
# CMake options
cmake_minimum_required(VERSION 2.8)

###############
# C++ options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")#-std=c++0x")
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/test)

###############
# import Boost
find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

###############
# import D-Bus
find_package(PkgConfig REQUIRED)
pkg_check_modules(DBus dbus-1)
include_directories(${DBus_INCLUDE_DIRS})
link_directories(${DBus_LIBRARY_DIRS})

##############
# import GTest
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

##############
# Tests
enable_testing()


add_executable(dbustests "test/avahi.cpp" "test/message.cpp")
target_link_libraries(dbustests ${Boost_LIBRARIES})
target_link_libraries(dbustests ${DBus_LIBRARIES})
target_link_libraries(dbustests ${GTEST_BOTH_LIBRARIES} gmock)
target_link_libraries(dbustests -pthread)
add_test(dbustests dbustests "--gtest_output=xml:${test_name}.xml")

