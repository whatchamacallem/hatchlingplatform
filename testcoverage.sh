#!/bin/sh
# SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
# SPDX-License-Identifier: MIT
# This file is licensed under the terms of the LICENSE.md file.

# POSIXLY_CORRECT=1 was breaking gcovr.
# Using a subdirectory was breaking gcovr. See "--root .. ."

set -o errexit

HX_DIR=`pwd`

# Build artifacts are not retained.
rm -rf ./bin; mkdir ./bin && cd ./bin

set -o xtrace

gcc -I$HX_DIR/include --coverage -O0 -g -DHX_RELEASE=0 -std=c99 \
	-c $HX_DIR/src/*.c $HX_DIR/test/*.c

g++ -I$HX_DIR/include --coverage -O0 -g -DHX_RELEASE=0 -DHX_TEST_ERROR_HANDLING=1 \
	-fno-exceptions -pthread -lpthread -std=c++11 -lstdc++ \
	$HX_DIR/src/*.cpp $HX_DIR/test/*.cpp *.o -o hxtest

echo runtests | ./hxtest help printhashes "checkhash 0" execstdin

gcovr --html-details coverage.html --root .. .

{ set +o xtrace; } 2> /dev/null

# Launch Chrome if it is installed.
if [ "$1" != "--headless" ] && which google-chrome; then
	google-chrome coverage.html >/dev/null 2>&1;
fi

# Make sure the script returns 0.
echo 🐉🐉🐉
