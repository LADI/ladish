#!/bin/sh

#echo 'Running slv2/autogen.sh...'
#cd slv2 && ./autogen.sh && cd ..

echo 'Generating necessary files...'
rm -rf slv2/config
mkdir -p slv2/config
rm -rf config
mkdir -p config
libtoolize --force
aclocal
autoheader -Wall
automake --foreign --add-missing -Wall
autoconf

