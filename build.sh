#!/usr/bin/bash

if [ "$#" -ne 1 ]; then
  echo "build.sh: usage ./build.sh [profile]" >&2
  exit 1
fi

PROFILE=$1

echo "Building for profile $PROFILE"

if [[ $PROFILE =~ "mingw" ]]; then
  PKG_CONFIG_PATH_CUSTOM="$(pwd)/build/$PROFILE" conan build . -of build/$PROFILE -pr:h $PROFILE -pr:b linux-gcc -o "boost/*:without_stacktrace=True" --build=missing
else
  conan build . -of build/$PROFILE -pr:a $PROFILE --build=missing
fi

