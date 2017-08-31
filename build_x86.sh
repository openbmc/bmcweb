#!/bin/sh

#Currently there are some build errors with the pre-instaleld libgtest-dev, as a workaround, we can build it with these steps(first time only):
#cd googletest/googletest/
#cmake ./
#make
#sudo make install

cd `dirname $0`
#update source code
#git pull
git submodule init
git submodule sync
git submodule update

mkdir -p build
cd build
cmake ..  -DHUNTER_ENABLED=ON
make  -j$(nproc)
