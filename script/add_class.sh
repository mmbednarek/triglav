#!/usr/bin/bash

CATEGORY=$1
MODULE=$2
CLASS_NAME=$3

if [ $# -lt 3 ]; then
  echo "Invalid number of args"
  exit 1
fi

echo "Copying files"

DST_HEADER="./$CATEGORY/$MODULE/include/triglav/$MODULE/$CLASS_NAME.hpp"
DST_SOURCE="./$CATEGORY/$MODULE/src/$CLASS_NAME.cpp"
cp ./script/template/class_header $DST_HEADER || exit 1
cp ./script/template/class_source $DST_SOURCE || exit 1

echo "Template replacement"

sed -i "s/%CLASS_NAME%/$CLASS_NAME/g" $DST_HEADER $DST_SOURCE || exit 1
sed -i "s/%MODULE_NAME%/$MODULE/g" $DST_HEADER $DST_SOURCE || exit 1

./script/list_source_files.sh $CATEGORY $MODULE