#!/bin/sh

set -e

if ! command -v docker > /dev/null; then
  echo "Please install docker from https://www.docker.com/products/docker-desktop"
  exit 1
fi

docker inspect bmcweb-base > /dev/null ||
  docker build --no-cache --force-rm -t bmcweb-base -f Dockerfile.base .

docker build -t bmcweb .

mkdir -p build

docker run -v $PWD:/app -it bmcweb cp -r /build/bmcweb /build/webtest /app/build
