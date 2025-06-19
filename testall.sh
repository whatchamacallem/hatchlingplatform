#!/bin/sh
# Copyright 2017-2025 Adrian Johnston
#
# Run all the build and test scripts for all targets. If bin/hxtest fails then
# the tests will stop at that point and it will be available to debug.

clear
echo Running all test scripts...

set -o errexit
set -x

time ./debugbuild.sh
time ./teststrip.sh
time ./test.sh
time ./testerrorhandling.sh
time ./testcmake.sh
time ./testcoverage.sh --headless
time ./testwasm.sh --headless
./clean.sh

echo All test scripts done.
