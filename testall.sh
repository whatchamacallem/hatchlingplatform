#!/bin/sh
# Copyright 2017-2025 Adrian Johnston
#
# Run all the build and test scripts for all targets. If bin/hxtest fails then
# the tests will stop at that point and it will be available to debug. Any
# shell command failing should also stop the test process.

clear
echo Running all test scripts...

set -o errexit
set -x

time ./debugbuild.sh
time ./teststrip.sh
time ./testmatrix.sh
time ./testerrorhandling.sh
time ./testcmake.sh
time ./testcoverage.sh --headless
time ./testwasm.sh --headless
./clean.sh

{ set +x; } 2> /dev/null
echo All test scripts done.
