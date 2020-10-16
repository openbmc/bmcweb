# Please build bmcweb-base first with
# docker build --no-cache --force-rm -t bmcweb-base -f Dockerfile.base .
FROM bmcweb-base

ADD . /source

RUN cd source && CXX=/clang_10/bin/clang++ meson setup build-clang

RUN cd source && CXX=c++ meson setup build-gcc

WORKDIR /source

