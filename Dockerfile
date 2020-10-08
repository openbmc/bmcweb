# Please build bmcweb-base first with
# docker build --no-cache --force-rm -t bmcweb-base -f Dockerfile.base .
FROM bmcweb-base

ADD . /source

RUN ninja -C /source/build

RUN mkdir -p /usr/share/www

CMD ["/build/bmcweb"]
