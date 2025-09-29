#!/bin/sh
# SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
# SPDX-License-Identifier: MIT
# This file is licensed under the terms of the LICENSE.md file.

set -o errexit

# test configuration options in ways that are not varied by the release level
# during normal testing.
echo "NOTA BENE: These tests will spew errors and still return successfully."
./testmatrix.sh \
	'-DHX_MEMORY_MANAGER_DISABLE=(HX_RELEASE==2)' \
	'-DHX_TEST_ERROR_HANDLING=1'
