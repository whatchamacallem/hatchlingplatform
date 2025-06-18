#!/bin/sh
# Copyright 2017-2025 Adrian Johnston

set -o errexit
set -o xtrace

rm -f profile.json hxConsoleTest_FileTest.txt hxFileTest_Operators.bin \
		hxFileTest_ReadWrite.txt *.o hxtest hxtest-strip \
		index.* *.html coverage.css *.gcda *gcno hatchlingPch.hpp.pch
