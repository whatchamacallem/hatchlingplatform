#!/bin/sh
# SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
# SPDX-License-Identifier: MIT
# This file is licensed under the terms of the LICENSE.md file.
#
# Run all the build and test scripts for all targets. If bin/hxtest fails, then
# the tests will stop at that point and it will be available to debug. Any
# shell command that fails should also stop the test process.

clear

echo "
$(tput rev)
.                                                                             .
.   ██╗  ██╗ █████╗ ████████╗ ██████╗██╗  ██╗██╗     ██╗███╗   ██╗ ██████╗    .
.   ██║  ██║██╔══██╗╚══██╔══╝██╔════╝██║  ██║██║     ██║████╗  ██║██╔════╝    .
.   ███████║███████║   ██║   ██║     ███████║██║     ██║██╔██╗ ██║██║  ███╗   .
.   ██╔══██║██╔══██║   ██║   ██║     ██╔══██║██║     ██║██║╚██╗██║██║   ██║   .
.   ██║  ██║██║  ██║   ██║   ╚██████╗██║  ██║███████╗██║██║ ╚████║╚██████╔╝   .
.   ╚═╝  ╚═╝╚═╝  ╚═╝   ╚═╝    ╚═════╝╚═╝  ╚═╝╚══════╝╚═╝╚═╝  ╚═══╝ ╚═════╝    .
.    ██████╗ ██╗      █████╗ ████████╗███████╗ ██████╗ ██████╗ ███╗   ███╗    .
.    ██╔══██╗██║     ██╔══██╗╚══██╔══╝██╔════╝██╔═══██╗██╔══██╗████╗ ████║    .
.    ██████╔╝██║     ███████║   ██║   █████╗  ██║   ██║██████╔╝██╔████╔██║    .
.    ██╔═══╝ ██║     ██╔══██║   ██║   ██╔══╝  ██║   ██║██╔══██╗██║╚██╔╝██║    .
.    ██║     ███████╗██║  ██║   ██║   ██║     ╚██████╔╝██║  ██║██║ ╚═╝ ██║    .
.    ╚═╝     ╚══════╝╚═╝  ╚═╝   ╚═╝   ╚═╝      ╚═════╝ ╚═╝  ╚═╝╚═╝     ╚═╝    .
.                                                                             .
$(tput sgr 0)
"

# Delete files matching .gitignore and reset ccache.
./clean.sh

set -o errexit -o xtrace

time ./debugbuild.sh --headless
time ./testcmake.sh
time ./testcoverage.sh --headless
time ./testerrorhandling.sh
time ./testmatrix.sh
time ./testpythonbindings.sh
time ./teststrip.sh
time ./testwasm.sh --headless
time doxygen

{ set +o xtrace; } 2> /dev/null
./clean.sh
echo All test scripts done.
