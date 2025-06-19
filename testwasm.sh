#!/bin/sh
#
# After building the emsdk these commands need to be run in the emsdk directory:
#
#   ./emsdk activate latest && source ./emsdk_env.sh

export POSIXLY_CORRECT=1

# Use the sort command to do a version aware comparison of two strings on two
# different lines.
echo "emcc (Emscripten gcc/clang-like replacement + linker emulating GNU ld) 4.0.0
`emcc --version | grep emcc`" | sort -V -c 2>/dev/null

# sort returns 1 if the emcc version is too small.
if [ $? -eq 0 ]; then
	echo "emcc version is ok."
else
	echo "ERROR: emcc is missing or emcc version is too small."
	echo "see: https://emscripten.org/docs/getting_started/downloads.html"
	exit 1;
fi

set -o errexit
set -m # job control

# Build artifacts are not retained.
rm -rf ./bin
mkdir ./bin
cd ./bin

emcc -I../include -O2 -fpic -fdiagnostics-absolute-paths -c ../src/*.c ../test/*.c

# -sMAIN_MODULE=2 dead-strips without leaving code for other modules.
# Dump the memory manager because a web browser doesn't need that.
emcc -I../include -O2 -fpic -sMAIN_MODULE=2 -fno-exceptions -fno-rtti \
	-DHX_MEMORY_MANAGER_DISABLE=1 -fdiagnostics-absolute-paths \
	*.o ../*/*.cpp -o index.html

if [ "$1" != "--headless" ]; then

	# Start a webserver in the background.
	python3 -m http.server 9876 &

	# Launch Chrome if it is installed.
	if which google-chrome >/dev/null 2>&1 && [ "$1" != "--headless" ]; then
		google-chrome http://0.0.0.0:9876/ >/dev/null 2>&1;
	fi

	# Bring the webserver to the foreground so it can be killed.
	fg %$(jobs | grep 'python3' | sed -E 's/^\[([0-9]+)\].*/\1/')
fi

# Say goodbye. Make sure the script returns 0.
echo 🐉🐉🐉
