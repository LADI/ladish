#!/bin/sh

aclocal-1.9
#autoheader -Wall
automake-1.9 --gnu --add-missing -Wall
autoconf -Wall
