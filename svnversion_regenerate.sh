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

echo "#define ${DEFINE} \""`svnversion`"\"" > ${TEMP_FILE}
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
