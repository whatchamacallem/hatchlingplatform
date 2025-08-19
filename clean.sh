#!/bin/sh
# SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
# SPDX-License-Identifier: MIT
# This file is licensed under the terms of the LICENSE.md file.

rm -rf ./bin

# Clean up after Python package build. This is how Python likes it.
rm -rf	entanglement_example/build \
		entanglement_example/entanglement_example.egg-info \
		entanglement_example/src/entanglement_example.py \
		entanglement_example/src/libentanglement_example.so.1
