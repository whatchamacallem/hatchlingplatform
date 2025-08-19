#!/bin/sh
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

sudo apt install -y \
	clang           \
	cmake           \
	g++             \
	g++-multilib    \
	gcc-multilib    \
	gcovr           \
	gdb             \
	gdb-multiarch   \
	llvm            \
	llvm-18         \
	llvm-18-dev     \
	musl            \
	musl-dev        \
	musl-tools

DIV='║▌║█║▌│║▌║▌█║▌║█║▌│║▌║▌█║▌║█║▌│║▌║▌█║▌║█║▌│║▌║▌█║▌║█║▌│║▌║▌█║▌║█║▌│║▌║▌█'

echo $DIV
gcc --version
echo $DIV
musl-gcc --version
echo $DIV
clang  --version
echo $DIV
emcc --version
echo $DIV
gcovr --version
echo $DIV
cmake --version
echo $DIV
python3  --version
echo $DIV
