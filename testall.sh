#!/bin/dash
# Copyright 2017-2025 Adrian Johnston
#
# Run all the build and test scripts for all targets. If bin/hxtest fails then
# the tests will stop at that point and it will be available to debug.

set -o errexit
set -x

time ./debugbuild.sh
time ./teststrip.sh
time ./test.sh
time ./testerrorhandling.sh
time ./testcmake.sh
time ./testcoverage.sh
time ./testwasm.sh
./clean.sh

{ set +x; } 2> /dev/null
echo testall.sh successful.
