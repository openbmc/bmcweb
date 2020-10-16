#!/bin/sh

set -e

if ! command -v docker > /dev/null; then
  echo "Please install docker from https://www.docker.com/products/docker-desktop"
  exit 1
fi

docker inspect bmcweb-base > /dev/null ||
   docker build --network=host --force-rm -t bmcweb-base -f Dockerfile.base .

if [ $# -eq 0 ]
  then
    echo "No arguments supplied"
    exit 1
fi

if [[ "$*" == run-clang-tidy ]]
then
    echo "option selected : clang-tidy"
    docker build -t bmcweb .
    docker run -it bmcweb /clang_10/share/clang/run-clang-tidy.py -p build-clang/ -clang-tidy-binary /clang_10/bin/clang-tidy
    
else
    echo "option selected : build-bmcweb"
    docker build -t bmcweb .
    docker run -v "$PWD":/app -it bmcweb ninja -C build-gcc
    docker run -v "$PWD":/app -it bmcweb cp -rf /source/build-gcc/ /app/build
fi

