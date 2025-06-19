#!/bin/sh

export POSIXLY_CORRECT=1

set -o errexit

HX_DIR=`pwd`

# Build artifacts are not retained.
rm -rf ./bin
mkdir ./bin
cd ./bin

set -x

gcc -I$HX_DIR/include --coverage -O0 -DHX_RELEASE=0 -std=c99 -ggdb3 \
	-c $HX_DIR/src/*.c $HX_DIR/test/*.c

g++ -I$HX_DIR/include --coverage -O0 -DHX_RELEASE=0 -DHX_TEST_ERROR_HANDLING=1 \
	-fno-exceptions -pthread -lpthread -std=c++17 -lstdc++ -ggdb3 \
	$HX_DIR/*/*.cpp *.o -o hxtest

echo runtests | ./hxtest printhashes help "checkhash 0" execstdin

gcovr --html-details coverage.html

# turn off tracing silently.
{ set +x; } &>/dev/null

# Launch Chrome if it is installed.
[ "$1" != "--headless" ] && [ -x /usr/bin/google-chrome ] \
	&& /usr/bin/google-chrome coverage.html &>/dev/null

# Make sure the script returns 0.
echo 🐉🐉🐉
