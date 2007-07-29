#!/bin/sh

#echo 'Running slv2/autogen.sh...'
#cd slv2 && ./autogen.sh && cd ..

echo 'Generating necessary files...'
mkdir -p config
libtoolize --force
aclocal
autoheader -Wall
automake --foreign --add-missing -Wall
autoconf

