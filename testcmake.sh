#!/bin/sh
# Copyright 2017-2025 Adrian Johnston

export POSIXLY_CORRECT=1

set -o errexit

# Build artifacts are not retained.
rm -rf ./bin
mkdir ./bin
cd ./bin

set -x

cmake ..
make
./hxtest

# turn off tracing silently and make sure the command returns 0.
{ set +x; } 2> /dev/null
echo 🐉🐉🐉
