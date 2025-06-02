#!/bin/bash
#
# After building the emsdk these commands need to be run in the emsdk directory:
#
#   ./emsdk activate latest
#   source ./emsdk_env.sh

# Use the sort command to do a version aware comparison of two strings on two
# different lines.
echo "emcc (Emscripten gcc/clang-like replacement + linker emulating GNU ld) 4.0.0
`emcc --version | grep emcc`" | sort -V -c 2>/dev/null

# sort returns 1 if the emcc version is too small.
if [ ${PIPESTATUS[1]} -eq 0 ]; then
	echo "emcc version is ok."
else
	echo "ERROR: emcc is missing or emcc version is too small."
	echo "see: https://emscripten.org/docs/getting_started/downloads.html"
	exit 1;
fi

set -o errexit

emcc -Iinclude -O2 -c src/*.c test/*.c
emcc -Iinclude -O2 -fno-exceptions *.o */*.cpp -o index.html

python3 -m http.server 9876
