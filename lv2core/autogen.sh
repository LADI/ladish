#!/bin/sh

echo 'Generating necessary files...'
rm -rf config
mkdir -p config
aclocal
automake --foreign --add-missing -Wall
autoconf
