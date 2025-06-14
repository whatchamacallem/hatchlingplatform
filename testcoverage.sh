#!/bin/bash
set -o errexit
set -o xtrace

gcc -Iinclude --coverage -O0 -DHX_RELEASE=0 -std=c99 -c src/*.c test/*.c

g++ -Iinclude --coverage -O0 -DHX_RELEASE=0 -DHX_TEST_ERROR_HANDLING=1 \
	-fno-exceptions -pthread -lpthread -std=c++17 -lstdc++ \
	*/*.cpp *.o -o hxtest

echo runtests | ./hxtest printhashes help execstdin

gcovr --html-details coverage.html

# turn off tracing silently.
{ set +x; } 2> /dev/null

# Launch Chrome if it is installed.
[ -x /usr/bin/google-chrome ] && /usr/bin/google-chrome coverage.html

# Make sure the script returns 0.
echo 🐉🐉🐉
