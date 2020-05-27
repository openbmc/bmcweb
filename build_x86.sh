#!/bin/sh
sudo apt-get install -y libpam0g-dev libssl-dev zlib1g-dev
pip install meson --user
meson builddir -Dyocto-deps=enabled
ninja -C builddir
