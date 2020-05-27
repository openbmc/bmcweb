#!/bin/sh
sudo apt-get install -y libpam0g-dev libssl-dev zlib1g-dev
meson builddir -Dyocto-deps=enabled
ninja -C builddir
