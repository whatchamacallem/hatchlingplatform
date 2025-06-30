#!/bin/sh
# Copyright 2017-2025 Adrian Johnston

# Only rerun cmake manually as is customary. Build artifacts ARE retained.

set -o errexit

export POSIXLY_CORRECT=1

if [ ! -f "bin/Makefile" ]; then
	rm -rf ./bin; mkdir ./bin && cd ./bin
	cmake ..
else
	cd ./bin
fi

make
./hxtest

echo 🐉🐉🐉
