cmake_minimum_required(VERSION 2.8.10 FATAL_ERROR)

cmake_policy(SET CMP0054 OLD)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

message("${CMAKE_MODULE_PATH}")

SET(BUILD_SHARED_LIBRARIES OFF)

#set(HUNTER_ROOT /home/ed/hunter)
#SET(HUNTER_STATUS_DEBUG ON)


#SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls")

#SET(MSAN_CXX_FLAGS "-fsanitize=memory -fsanitize-memory-track-origins=2 -fno-omit-frame-pointer -stdlib=libc++ -I/home/ed/libcxx_msan/include -I/home/ed/libcxx_msan/include/c++/v1")
#SET(MSAN_LINKER_EXE_FLAGS "${MSAN_CXX_FLAGS} -lc++abi -L/home/ed/libcxx_msan/lib")
include("cmake/HunterGate.cmake")
#HunterGate(
#    URL "https://github.com/ruslo/hunter/archive/v0.16.31.tar.gz"
#    SHA1 "8fcc0a2d6206e1f2c6fc011e3e694e388d030b53"
#)
HunterGate(
    URL "https://github.com/ruslo/hunter/archive/v0.18.16.tar.gz"
    SHA1 "6cbca2b0e7605ad8ea22ee3527850996436f71b8"
)


option(BUILD_FOR_EMBEDDED "Build for device target" ON)

project(bmc-webserver CXX C)

SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${MSAN_CXX_FLAGS}")
SET(CMAKE_EXE_LINKER_FLAGS "${MSAN_LINKER_EXE_FLAGS}")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if ( ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR} )
    message( FATAL_ERROR "In-source builds not allowed. Please make a new directory (usually called build) and run CMake from there. You may need to remove CMakeCache.txt." )
endif()

# general
option(BUILD_SHARED_LIBS "Build as shared library" OFF)
option(BUILD_UT "Enable Unit test" ON)

# This needs to be before the crow and other module includes so headers get overriden correctly
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# security flags
#SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fstack-protector-strong -fPIE -fPIC -D_FORTIFY_SOURCE=2 -Wformat -Wformat-security" )
#SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -z noexecstack -z relro -z now")

# Boost
#add_definitions(-DBOOST_ERROR_CODE_HEADER_ONLY)
#add_definitions(-DBOOST_ASIO_ENABLE_HANDLER_TRACKING)
#add_definitions(-DBOOST_ERROR_CODE_HEADER_ONLY)
add_definitions(-DBOOST_ALL_NO_LIB)
set(Boost_USE_STATIC_LIBS ON)
hunter_add_package(Boost COMPONENTS system thread)
find_package(Boost REQUIRED system thread REQUIRED)

#Openssl
hunter_add_package(OpenSSL)
find_package(OpenSSL REQUIRED)



#g3 logging
# G3logger does some unfortunate compile options, so cheat a little bit and copy/paste
set(LOG_SRC ${CMAKE_CURRENT_SOURCE_DIR}/g3log/src)

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
if(("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU") OR ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"))
    set_source_files_properties(g3log/src/logcapture.cpp PROPERTIES COMPILE_FLAGS -Wno-braced-scalar-init)
    set_source_files_properties(g3log/src/filesink.cpp PROPERTIES COMPILE_FLAGS -Wno-braced-scalar-init)
    set_source_files_properties(g3log/src/logworker.cpp PROPERTIES COMPILE_FLAGS -Wno-braced-scalar-init)
endif()

set_target_properties(g3logger PROPERTIES
    LINKER_LANGUAGE CXX
    OUTPUT_NAME g3logger
    CLEAN_DIRECT_OUTPUT 1)
target_link_libraries(g3logger ${PLATFORM_LINK_LIBRIES})

#lib jpeg
set(BUILD_STATIC ON)
#include_directories(libjpeg)
#include_directories(${CMAKE_CURRENT_BINARY_DIR}/libjpeg)
#add_subdirectory(libjpeg)


# Crow
add_definitions(-DCROW_ENABLE_SSL)
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/crow/include)

#Zlib
hunter_add_package(ZLIB)
find_package(ZLIB REQUIRED)

# C++ GSL (Guideline support libraries)
include_directories(gsl-lite/include)

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

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0)
        set_source_files_properties(${CMAKE_BINARY_DIR}/generated/webassets.cpp PROPERTIES COMPILE_FLAGS -Wno-narrowing)
    endif(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0)
endif()


set(SRC_FILES
    src/token_authorization_middleware.cpp
    src/security_headers_middleware.cpp
    src/base64.cpp
    ${GENERATED_SRC_FILES}
)

set(UT_FILES
    g3log/test_unit/test_cpp_future_concepts.cpp   
    g3log/test_unit/tester_sharedlib.cpp  
    # TODO(ed) figure out why this unit test has a main function in it
    #g3log/test_unit/test_filechange.cpp  
    g3log/test_unit/testing_helpers.h  
    #TODO(ed)
    #g3log/test_unit/test_linux_dynamic_loaded_sharedlib.cpp  
    #g3log/test_unit/test_sink.cpp
    g3log/test_unit/test_concept_sink.cpp  
    g3log/test_unit/test_crashhandler_windows.cpp  
    g3log/test_unit/tester_sharedlib.h    
    g3log/test_unit/testing_helpers.cpp  
    g3log/test_unit/test_io.cpp        
    g3log/test_unit/test_message.cpp
    src/crowtest/crow_unittest.cpp
    src/gtest_main.cpp
    src/base64_test.cpp
    src/token_authorization_middleware_test.cpp
    src/security_headers_middleware_test.cpp
    src/webassets_test.cpp
    src/crow_getroutes_test.cpp
    src/ast_jpeg_decoder_test.cpp

    ${CMAKE_BINARY_DIR}/generated/blns.hpp
)


file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/generated")

file(COPY src/test_resources DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Unit Tests
if(${BUILD_UT})
    # big list of naughty strings
    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/generated/blns.hpp
        COMMAND xxd -i ${CMAKE_CURRENT_SOURCE_DIR}/src/blns.txt ${CMAKE_BINARY_DIR}/generated/blns.hpp)

    # googletest
    enable_testing()
    find_package(GTest REQUIRED)
    find_package(GMock REQUIRED)

    add_executable(unittest ${HDR_FILES} ${SRC_FILES} ${UT_FILES})
    target_link_libraries(unittest ${GMOCK_BOTH_LIBRARIES})
    target_link_libraries(unittest Boost::system Boost::thread) 
    target_link_libraries(unittest ${CMAKE_THREAD_LIBS_INIT})
    target_link_libraries(unittest OpenSSL::SSL OpenSSL::Crypto)
    target_link_libraries(unittest g3logger)
    target_link_libraries(unittest ${ZLIB_LIBRARIES})
    add_dependencies(unittest packagestaticcpp)
endif(${BUILD_UT})

# web static assets
add_subdirectory(static)

# bmcweb
add_executable(bmcweb ${WEBSERVER_MAIN} ${HDR_FILES} ${SRC_FILES})
target_link_libraries(bmcweb  Boost::system Boost::thread)
target_link_libraries(bmcweb ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(bmcweb OpenSSL::SSL OpenSSL::Crypto)
target_link_libraries(bmcweb g3logger)
target_link_libraries(bmcweb ${ZLIB_LIBRARIES})
add_dependencies(bmcweb packagestaticcpp)

# udpclient
add_executable(udpclient src/udpclient.cpp)

target_link_libraries(udpclient Boost::system)
target_link_libraries(udpclient ${CMAKE_THREAD_LIBS_INIT})

add_executable(getvideo src/getvideo_main.cpp)
target_link_libraries(getvideo ${CMAKE_THREAD_LIBS_INIT})

# Visual Studio Code helper
# this needs to be at the end to make sure all includes are handled correctly

get_property(C_INCLUDE_DIRS DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
execute_process(COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/scripts/prime_vscode_compile_db.py ${C_INCLUDE_DIRS} ${C_SYSTEM_INCLUDE_DIRS})