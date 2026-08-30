#!/usr/bin/bash

CATEGORY=$1
MODULE=$2

if [ $# -lt 2 ]; then
  echo "Invalid number of args"
  exit 1
fi

SOURCE_DIR="$CATEGORY/$MODULE/src"
INCLUDE_DIR="$CATEGORY/$MODULE/include/triglav/$MODULE"

HEADER_FILES=$(find $INCLUDE_DIR -name '*.hpp' | sort)
SOURCE_FILES=$(find $SOURCE_DIR -regextype posix-extended -regex '.*\.(hpp|cpp)' | sort)

HEADER_FILES_STRIPPED=()
SOURCE_FILES_STRIPPED=()

for HEADER in ${HEADER_FILES[@]}; do
  STRIPPED="${HEADER#$CATEGORY/$MODULE/}"
  HEADER_FILES_STRIPPED+=($STRIPPED)
done

for SOURCE in ${SOURCE_FILES[@]}; do
  STRIPPED="${SOURCE#$CATEGORY/$MODULE/}"
  SOURCE_FILES_STRIPPED+=($STRIPPED)
done

MESON_SOURCE="$CATEGORY/$MODULE/meson.build"
TMP_FILE="/tmp/meson.build"


awk '
/# !GENERATED/ { skip=1; next }
!skip
' $MESON_SOURCE >$TMP_FILE

echo "# !GENERATED" >>$TMP_FILE

for HEADER in ${HEADER_FILES_STRIPPED[@]}; do
  echo "        '$HEADER'," >>$TMP_FILE
done

for SOURCE in ${SOURCE_FILES_STRIPPED[@]}; do
  echo "        '$SOURCE'," >>$TMP_FILE
done

echo "# !END" >>$TMP_FILE

awk '
skip;
/# !END/       { skip=1 }
' $MESON_SOURCE >>$TMP_FILE

mv $TMP_FILE $MESON_SOURCE