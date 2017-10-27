cmake_minimum_required(VERSION 2.8.10 FATAL_ERROR)

cmake_policy(SET CMP0054 NEW)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
message(CMAKE_MODULE_PATH=${CMAKE_MODULE_PATH})

option(BUILD_STATIC_LIBS "Built static libraries" ON)

option(HUNTER_ENABLED "Enable hunter package pulling" OFF)

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

SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -Wall")

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-rtti")

# general
option(BMCWEB_BUILD_UT "Enable Unit test" ON)

# security flags
SET(SECURITY_FLAGS " -fstack-protector-strong -fPIE -fPIC -D_FORTIFY_SOURCE=2 -Wformat -Wformat-security")
SET(CMAKE_CXX_FLAGS_RELEASE  "${CMAKE_CXX_FLAGS_RELEASE} ${SECURITY_FLAGS}" )
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO  "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${SECURITY_FLAGS}" )
SET(CMAKE_C_FLAGS_MINSIZEREL  "${CMAKE_C_FLAGS_MINSIZEREL} ${SECURITY_FLAGS}" )

#add_definitions(-DBOOST_ASIO_ENABLE_HANDLER_TRACKING)
add_definitions(-DBOOST_ERROR_CODE_HEADER_ONLY)
add_definitions(-DBOOST_SYSTEM_NO_DEPRECATED)
add_definitions(-DBOOST_ALL_NO_LIB)
add_definitions(-DBOOST_NO_RTTI)
add_definitions(-DBOOST_NO_TYPEID)
#set(Boost_USE_STATIC_LIBS ON)
hunter_add_package(Boost)
find_package(Boost)
include_directories(${Boost_INCLUDE_DIRS}) 

#Openssl
hunter_add_package(OpenSSL)
find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})
message("OPENSSL_INCLUDE_DIR ${OPENSSL_INCLUDE_DIR}")

# Crow
#add_definitions(-DCROW_DISABLE_LOGGING)
add_definitions(-DCROW_ENABLE_SSL)
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/crow/include)

#Zlib
hunter_add_package(ZLIB)
find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})

#tinyxml2
add_subdirectory(tinyxml2)

# PAM
option(WEBSERVER_ENABLE_PAM "enable pam authentication" ON)
if ("${WEBSERVER_ENABLE_PAM}")
    find_package(PAM REQUIRED)
else()
    add_definitions("-DWEBSERVER_DISABLE_PAM")
endif()


# Boost-dbus
set(BOOST_DBUS_BUILD_UT ${BMCWEB_BUILD_UT})
add_subdirectory(boost-dbus)

set(WEBSERVER_MAIN src/webserver_main.cpp)

# the webassets file has a non-ideal beahvior, where it loads binary data into a std::string
# due to the way crow is constructed.  This causes GCC warnings.  Filter them out.
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.9)
        set_source_files_properties(${CMAKE_BINARY_DIR}/generated/webassets.cpp PROPERTIES COMPILE_FLAGS -Wno-narrowing)
    endif(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.9)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(SRC_FILES
    src/base64.cpp
    ${GENERATED_SRC_FILES}
)

file(COPY src/test_resources DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/generated)

# Unit Tests
if(${BMCWEB_BUILD_UT})
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
        src/msan_test.cpp
        src/ci_map_tests.cpp
        src/ast_video_puller_test.cpp
        ${CMAKE_BINARY_DIR}/generated/blns.hpp
    )
    # big list of naughty strings
    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/generated/blns.hpp
        COMMAND xxd -i ${CMAKE_CURRENT_SOURCE_DIR}/src/test_resources/blns ${CMAKE_BINARY_DIR}/generated/blns.hpp)
    
    set_source_files_properties(${CMAKE_BINARY_DIR}/generated/blns.hpp
        PROPERTIES GENERATED TRUE)
    # googletest
    enable_testing()

    hunter_add_package(GTest)
    find_package(GTest REQUIRED)
    #find_package(GMock CONFIG REQUIRED)

    add_executable(webtest ${SRC_FILES} ${UT_FILES})
    target_link_libraries(webtest gmock gtest)
    target_link_libraries(webtest pthread)
    target_link_libraries(webtest boost-dbus)
    target_link_libraries(webtest ${OPENSSL_LIBRARIES})
    target_link_libraries(webtest ${ZLIB_LIBRARIES})
    target_link_libraries(webtest pam)
    add_dependencies(webtest tinyxml2_static)
    target_link_libraries(webtest tinyxml2_static)
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
target_link_libraries(bmcweb -lstdc++fs)
add_dependencies(bmcweb tinyxml2_static)
target_link_libraries(bmcweb tinyxml2_static)
install(TARGETS bmcweb DESTINATION bin)

add_executable(getvideo src/getvideo_main.cpp)
target_link_libraries(getvideo pthread)

# Visual Studio Code helper
# this needs to be at the end to make sure all includes are handled correctly
include(CMakeExtraGeneratorDetermineCompilerMacrosAndIncludeDirs)
get_property(C_INCLUDE_DIRS DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)

execute_process(
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/scripts/prime_vscode_compile_db.py 
    ${C_INCLUDE_DIRS} 
    ${CMAKE_EXTRA_GENERATOR_C_SYSTEM_INCLUDE_DIRS} 
    ${CMAKE_EXTRA_GENERATOR_CXX_SYSTEM_INCLUDE_DIRS}
)
