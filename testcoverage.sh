#!/bin/sh

# POSIXLY_CORRECT=1 was breaking gcovr.
# Using a subdirectory was breaking gcovr. See "--root .. ."

set -o errexit

HX_DIR=`pwd`

# Build artifacts are not retained.
rm -rf ./bin
mkdir ./bin
cd ./bin

set -x

gcc -I$HX_DIR/include --coverage -Og -DHX_RELEASE=0 -std=c99 \
	-c $HX_DIR/src/*.c $HX_DIR/test/*.c

g++ -I$HX_DIR/include --coverage -Og -DHX_RELEASE=0 -DHX_TEST_ERROR_HANDLING=1 \
	-fno-exceptions -pthread -lpthread -std=c++11 -lstdc++ \
	$HX_DIR/*/*.cpp *.o -o hxtest

echo runtests | ./hxtest help printhashes "checkhash 0" execstdin

gcovr --html-details coverage.html --root .. .

# turn off tracing silently.
{ set +x; } 2> /dev/null

# Launch Chrome if it is installed.
if which google-chrome >/dev/null 2>&1 && [ "$1" != "--headless" ]; then
	google-chrome coverage.html >/dev/null 2>&1;
fi

# Make sure the script returns 0.
echo 🐉🐉🐉
