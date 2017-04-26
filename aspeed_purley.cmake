# this one is important
SET(CMAKE_SYSTEM_NAME Linux)
#this one not so much
SET(CMAKE_SYSTEM_VERSION 1)
SET(CMAKE_SYSTEM_PROCESSOR "armv6")
set(ARCH "armv6")

SET(CMAKE_CROSSCOMPILING True)

# specify the cross compiler
#SET(CMAKE_C_COMPILER arm-linux-gnueabi-gcc-4.9) 
#SET(CMAKE_CXX_COMPILER arm-linux-gnueabi-g++-4.9)
#SET(CMAKE_C_LINK_EXECUTABLE "clang <OBJECTS> -o  <TARGET> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <LINK_LIBRARIES>")
#SET(CMAKE_CXX_LINK_EXECUTABLE "clang++ <OBJECTS> -o  <TARGET> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <LINK_LIBRARIES>")

set(CMAKE_C_COMPILER /home/ed/deg-bmcfw-core/ToolChain/Host/AST2500/x-tools/arm-aspeed-linux-gnueabi/bin/arm-linux-gcc)
set(CMAKE_CXX_COMPILER /home/ed/deg-bmcfw-core/ToolChain/Host/AST2500/x-tools/arm-aspeed-linux-gnueabi/bin/arm-linux-g++)


set(triple arm-linux-gnueabi)
set(CMAKE_C_COMPILER_TARGET ${triple})
set(CMAKE_CXX_COMPILER_TARGET ${triple})

# where is the target environment 
SET(CMAKE_FIND_ROOT_PATH /home/ed/deg-bmcfw-core/_sysroot/AST2500)

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
