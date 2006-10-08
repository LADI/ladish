#!/bin/sh

top=`pwd`

echo 'Generating necessary files (recursively)...'

aclocal &

echo -n "* raul * \t"
cd raul
./autogen.sh
cd $top

echo -n "* flowcanvas * \t"
cd flowcanvas
./autogen.sh
cd $top

echo -n "* patchage * \t"
cd patchage
./autogen.sh
cd $top

echo -n "* slv2 * \t"
cd slv2
./autogen.sh
cd $top

echo -n "* omins * \t"
cd omins
./autogen.sh
cd $top

echo -n "* ingen * \t"
cd ingen
./autogen.sh
cd $top

automake --foreign --add-missing
autoconf

