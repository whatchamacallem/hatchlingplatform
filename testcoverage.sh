#!/bin/dash

export POSIXLY_CORRECT=1

set -o errexit

# Build artifacts are not retained.
rm -rf ./bin
mkdir ./bin
cd ./bin

set -o xtrace

gcc -I../include --coverage -O0 -DHX_RELEASE=0 -std=c99 -c ../src/*.c ../test/*.c

g++ -I../include --coverage -O0 -DHX_RELEASE=0 -DHX_TEST_ERROR_HANDLING=1 \
	-fno-exceptions -pthread -lpthread -std=c++17 -lstdc++ \
	../*/*.cpp *.o -o hxtest

echo runtests | ./hxtest printhashes help "checkhash 0" execstdin

gcovr --html-details coverage.html

# turn off tracing silently.
{ set +x; } &>/dev/null

# Launch Chrome if it is installed.
[ -x /usr/bin/google-chrome ] && /usr/bin/google-chrome coverage.html &>/dev/null

# Make sure the script returns 0.
echo 🐉🐉🐉
