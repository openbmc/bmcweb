cmake_minimum_required(VERSION 2.8.10 FATAL_ERROR)

cmake_policy(SET CMP0054 OLD)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

message("${CMAKE_MODULE_PATH}")

#set(HUNTER_ROOT /home/ed/hunter)
#SET(HUNTER_STATUS_DEBUG ON)
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
option(BUILD_SHARED_LIBS "Build as shared library" OFF)
option(BUILD_UT "Enable Unit test" OFF)

# Boost
add_definitions(-DBOOST_ASIO_ENABLE_HANDLER_TRACKING)
add_definitions(-DBOOST_ALL_NO_LIB)
set(Boost_USE_STATIC_LIBS ON)
hunter_add_package(Boost COMPONENTS system thread)
find_package(Boost REQUIRED system thread REQUIRED)

#Openssl
hunter_add_package(OpenSSL)
find_package(OpenSSL REQUIRED)

# Crow
add_definitions(-DCROW_ENABLE_SSL)
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/crow/include)
#add_subdirectory(crow)

#g3 logging
option(ADD_FATAL_EXAMPLE "Disable g3 examples" OFF)
add_subdirectory(g3log)
include_directories(g3log/src)

#Zlib
#hunter_add_package(ZLIB)
#find_package(ZLIB REQUIRED)

# Debug sanitizers
find_package(Sanitizers)

# C++ GSL (Guideline support libraries)
include_directories(gsl-lite/include)

set(WEBSERVER_MAIN src/webserver_main.cpp)

set(HDR_FILES
    include/crow_g3_logger.hpp
    include/ssl_key_handler.hpp
    include/color_cout_g3_sink.hpp
    include/webassets.hpp
)

set(GENERATED_SRC_FILES 
    ${CMAKE_BINARY_DIR}/generated/webassets.cpp
)

if (CMAKE_COMPILER_IS_GNUCC)
    execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion
                    OUTPUT_VARIABLE GCC_VERSION)
    string(REGEX MATCHALL "[0-9]+" GCC_VERSION_COMPONENTS ${GCC_VERSION})
    list(GET GCC_VERSION_COMPONENTS 0 GCC_MAJOR)
    list(GET GCC_VERSION_COMPONENTS 1 GCC_MINOR)

    message(STATUS ${GCC_MAJOR})
    message(STATUS ${GCC_MINOR})
endif()

set_source_files_properties(${GENERATED_SRC_FILES} PROPERTIES GENERATED TRUE)

set(SRC_FILES
    src/token_authorization_middleware.cpp
    src/base64.cpp
    ${GENERATED_SRC_FILES}
)

set(UT_FILES
    src/gtest_main.cpp
    src/base64_test.cpp
    src/token_authorization_middleware_test.cpp
    ${CMAKE_BINARY_DIR}/generated/blns.hpp
)

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/generated")

# avoid the "narrowing char to X" warning for negative numbers
# TODO, make the python just do the right thing and turn the number negative
# Background:  char is faster than unsigned char once compiled, so it is used 
# extensively in the server for the "byte" type.  Unfortunately, this means that
# the call std::string s{0xFF} fails, because 0xFF narrows to a negative number.
# This line simply disables the warning, and the compiler does the right thing
# we selectively only disable this warning on this specific file
# http://stackoverflow.com/questions/28094263/create-array-of-chars-avoiding-narrowing
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set_source_files_properties(${CMAKE_BINARY_DIR}/generated/webassets.cpp PROPERTIES COMPILE_FLAGS -Wno-c++11-narrowing)
endif()
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 5.2)
        set_source_files_properties(${CMAKE_BINARY_DIR}/generated/webassets.cpp PROPERTIES COMPILE_FLAGS -Wno-narrowing)
    endif(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 5.2)
endif()

# Unit Tests
if(${BUILD_UT})

    # big list of naughty strings
    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/generated/blns.hpp
        COMMAND xxd -i ${CMAKE_CURRENT_SOURCE_DIR}/src/blns.txt ${CMAKE_BINARY_DIR}/generated/blns.hpp)

    # googletest
    enable_testing()
    find_package(GTest REQUIRED)

    add_executable(unittest ${HDR_FILES} ${SRC_FILES} ${UT_FILES})
    target_link_libraries(unittest GTest::GTest GTest::Main)
    target_link_libraries(unittest ${Boost_LIBRARIES} Boost::system)
    target_link_libraries(unittest ${CMAKE_THREAD_LIBS_INIT})
    target_link_libraries(unittest OpenSSL::SSL OpenSSL::Crypto)
    target_link_libraries(unittest g3logger)
    #target_link_libraries(unittest zlib)
    add_dependencies(unittest packagestaticcpp)
endif(${BUILD_UT})

# web static assets
add_subdirectory(static)

# bmcweb
add_executable(bmcweb ${WEBSERVER_MAIN} ${HDR_FILES} ${SRC_FILES})
target_link_libraries(bmcweb ${Boost_LIBRARIES} Boost::system)
target_link_libraries(bmcweb ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(bmcweb OpenSSL::SSL OpenSSL::Crypto)
target_link_libraries(bmcweb g3logger)
target_link_libraries(bmcweb ${ZLIB_LIBRARIES})
add_dependencies(bmcweb packagestaticcpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Visual Studio Code helper
# this needs to be at the end to make sure all includes are handled correctly
get_property(C_INCLUDE_DIRS DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
execute_process(COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/scripts/prime_vscode_compile_db.py ${C_INCLUDE_DIRS})
