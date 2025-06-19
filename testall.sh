#!/bin/dash
# Copyright 2017-2025 Adrian Johnston
#
# Run all the build and test scripts for all targets. If bin/hxtest fails then
# the tests will stop at that point and it will be available to debug.

set -o errexit
set -x

./debugbuild.sh
./teststrip.sh
./test.sh
./testerrorhandling.sh
./testcmake.sh
./testcoverage.sh
./testwasm.sh
./clean.sh
