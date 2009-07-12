#!/bin/sh

echo -n "Generating build files... "

if ! test -e ltmain.sh; then
  if test -e /usr/share/libtool/ltmain.sh; then
    ln -s /usr/share/libtool/ltmain.sh .;
  else
    echo "could not link ltmain.sh to this directory; find it and copy/link it to here";
  fi
fi

touch NEWS README ChangeLog

libtoolize --copy --force \
  && aclocal -I m4 \
  && autoheader \
  && automake --gnu --add-missing \
  && autoconf

#if test x$1 != x--no-conf; then
#  if test -e conf; then
#    sh conf;
#  fi
#fi

echo "done"
