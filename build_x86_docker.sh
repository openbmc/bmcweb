#!/bin/sh

set -e

docker inspect bmcweb-base > /dev/null || docker build --no-cache --force-rm -t bmcweb-base -f Dockerfile.base .

docker build -t bmcweb .

mkdir -p build

docker run -v $PWD:/app -it bmcweb cp -r /build/bmcweb /build/webtest /app/build
