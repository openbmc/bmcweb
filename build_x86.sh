#!/bin/sh
sudo apt-get install -y libpam0g-dev libssl-dev zlib1g-dev
mkdir -p build
cd build
cmake ..
cmake --build .
