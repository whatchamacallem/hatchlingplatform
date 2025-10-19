#!/bin/sh
# SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
# SPDX-License-Identifier: MIT
# This file is licensed under the terms of the LICENSE.md file.

# Only rerun cmake manually as is customary. Build artifacts ARE retained.

set -o errexit

export POSIXLY_CORRECT=1

if [ ! -f bin/build.ninja ]; then
	rm -rf bin
	cmake -S . -B bin -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
else
	echo found bin/build.ninja...
fi

ninja -C bin

echo run ./hxtest...
cd bin
./hxtest
cd ..

echo run-clang-tidy...
run-clang-tidy -quiet -p bin src/*.cpp src/*.c test/*.cpp

echo 🐉🐉🐉
