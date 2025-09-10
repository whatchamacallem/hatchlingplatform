#!/bin/bash
# SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
# SPDX-License-Identifier: MIT
# This file is licensed under the terms of the LICENSE.md file.
#
# Building and running all the tests on Ubuntu 24 LTS requires these packages.
#
# It should be safe to just run this script on Ubuntu. gdb-multiarch recently
# became required for vscode to set 32-bit breakpoints reliably. vscode is
# recommended because it is already configured but is not required or installed
# here. google-chrome will also be launched when a web page is generated if it
# is available but is not required or installed here.
#
# Please open a PR if anything is missing or has changed.
set -o errexit

# bash only line art. Can be run from the console as a work of art.
# Try: $ clear && separator 40
colors=(208 214 220 226 190 154 118 82 83 84 85 86 87 45 39 33
		27 33 39 45 87 86 85 84 83 82 118 154 190 226 220 214)
total_colors=${#colors[@]}
color_offset=0

separator() {
	local lines=${1:-1} # Read arg 1, defaults to 1.
    for((n=lines; n--;)); do
		for((i=40; i--;)); do
			local color_index=$(((color_offset + i) % total_colors))
			echo -ne "\033[38;5;${colors[color_index]}m──"
		done
		echo -e "\033[0m"
		((color_offset += 3))
	done
}

separator

c=$(echo "sudo apt install -y                              \
	clang         cmake         doxygen       g++          \
	g++-multilib  gcc-multilib  gcovr         gdb          \
	gdb-multiarch llvm          llvm-18       llvm-18-dev  \
	musl          musl-dev      musl-tools                 \
" | tr -s '[:space:]' ' ')
echo "\$ $c"
eval "$c"

separator

set +o errexit

for c in 'clang' 'cmake' 'doxygen' 'emcc' 'gcc' 'gcovr' 'musl-gcc' 'python3'
do
	echo "\$ $c --version"
    eval "$c --version"
	separator
done
