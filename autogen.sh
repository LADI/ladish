#!/bin/sh

top=`pwd`

echo 'Generating necessary files (recursively)...'

aclocal &

echo -ne "* raul * \t"
cd raul
./autogen.sh
cd $top

echo -ne "* flowcanvas * \t"
cd flowcanvas
./autogen.sh
cd $top

echo -ne "* patchage * \t"
cd patchage
./autogen.sh
cd $top

echo -ne "* slv2 * \t"
cd slv2
./autogen.sh
cd $top

echo -ne "* omins * \t"
cd omins
./autogen.sh
cd $top

echo -ne "* ingen * \t"
cd ingen
./autogen.sh
cd $top

automake --foreign --add-missing
autoconf

