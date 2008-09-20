#!/bin/sh

#set -x

if test $# -ne 1 -a $# -ne 2
then
  echo "Usage: "`basename "$0"`" <file> [define_name]"
  exit 1
fi

OUTPUT_FILE="${1}"
TEMP_FILE="${OUTPUT_FILE}.tmp"

if test $# -eq 2
then
  DEFINE=${2}
else
  DEFINE=SVN_VERSION
fi

if test -d .svn
then
  SVNVERSION=`svnversion`
else
  if test -d .git
  then
    git status >/dev/null # updates dirty state
    SVNVERSION=`git show | grep '^ *git-svn-id:' | sed 's/.*@\([0-9]*\) .*/\1/'`
    if test ${SVNVERSION}
    then
      test -z "$(git diff-index --name-only HEAD)" || SVNVERSION="${SVNVERSION}M"
    else
      SVNVERSION=0+`git rev-parse HEAD`
      test -z "$(git diff-index --name-only HEAD)" || SVNVERSION="${SVNVERSION}-dirty"
    fi
  fi
fi

if test -z ${SVNVERSION}
then
  SVNVERSION=exported
fi

echo "#define ${DEFINE} \"${SVNVERSION}\"" > ${TEMP_FILE}
if test ! -f ${OUTPUT_FILE}
then
  echo "Generated ${OUTPUT_FILE}"
  cp ${TEMP_FILE} ${OUTPUT_FILE}
  if test $? -ne 0; then exit 1; fi
else
  if ! cmp -s ${OUTPUT_FILE} ${TEMP_FILE}
  then echo "Regenerated ${OUTPUT_FILE}"
    cp ${TEMP_FILE} ${OUTPUT_FILE}
    if test $? -ne 0; then exit 1; fi
  fi
fi

rm ${TEMP_FILE}

exit $?
