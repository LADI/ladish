#!/bin/sh

echo 'Generating necessary files...'
rm -rf lv2core/config
mkdir -p lv2core/config
rm -rf slv2/config
mkdir -p slv2/config
rm -rf redlandmm/config
mkdir -p redlandmm/config
rm -rf raul/config
mkdir -p raul/config
rm -rf flowcanvas/config
mkdir -p flowcanvas/config
rm -rf config
mkdir -p config
rm -rf doc
mkdir -p doc
libtoolize --force
aclocal
autoheader -Wall
automake --foreign --add-missing -Wall -Wno-portability
autoconf

