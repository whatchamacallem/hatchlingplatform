#!/bin/bash
# Copyright 2017-2025 Adrian Johnston

dpkg --get-selections clang cmake g++ g++-multilib gcc-multilib gcovr python3 \
    |& grep -e "no packages"

# grep returns 1 if the pattern is not found.
if [ ${PIPESTATUS[1]} -eq 1 ]; then
	echo "All necessary packages installed."
    exit 0;
else
	echo "ERROR: missing packages."
    exit 1;
fi
