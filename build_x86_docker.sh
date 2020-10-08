#!/bin/sh

set -e

if ! command -v docker > /dev/null; then
  echo "Please install docker from https://www.docker.com/products/docker-desktop"
  exit 1
fi

docker inspect bmcweb-base > /dev/null ||
  docker build --network=host --no-cache --force-rm -t bmcweb-base -f Dockerfile.base .

docker build -t bmcweb .

docker run -v "$PWD":/app -it bmcweb cp -rf /source/build/ /app/build
