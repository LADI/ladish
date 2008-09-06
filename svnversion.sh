#!/bin/sh

if test $# -eq 1
then
  DIR=${1}
else
  DIR=.
fi

if test -d ${DIR}/.svn
then
    pushd ${DIR} > /dev/null
    svnversion
    popd  > /dev/null
else
    sed 's/^#define SVN_VERSION "\(.*\)"$/\1/' ${DIR}/svnversion.h
fi
