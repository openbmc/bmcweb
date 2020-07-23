#!/bin/sh

sudo apt-get install -y libpam0g-dev libssl-dev zlib1g-dev \
                        autoconf-archive autoconf

mkdir -p build
cd build
cmake ..
cmake --build .
