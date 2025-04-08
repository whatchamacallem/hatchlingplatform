#!/bin/bash
# Copyright 2017-2025 Adrian Johnston
#
# Google Test and emcc need to be downloaded separately.

dpkg --get-selections clang cmake g++ g++-multilib gcc-multilib gcovr python3 \
	|& grep -e "no packages"

# grep returns 1 if the pattern is not found.
if [ ${PIPESTATUS[1]} -eq 1 ]; then
	echo "All necessary packages installed."
else
	echo "ERROR: missing packages."
	exit 1;
fi

which emcc > /dev/null
if [ $? -eq 1 ]; then
	echo "emcc not available. see https://emscripten.org/docs/getting_started/downloads.html"
	exit 1;
fi

