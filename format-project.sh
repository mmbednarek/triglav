#!/usr/bin/env bash

clang-format -i $(find game library tool -regextype posix-extended -regex '^.*\.(cpp|h|hpp)$')

