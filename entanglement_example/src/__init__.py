# SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
# SPDX-License-Identifier: MIT
# This file is licensed under the terms of the LICENSE.md file.

__all__ = ['main', 'run_all_tests']  # implements: from entanglement_test import *

# Expose package interface.
from .entanglement import main
from .entanglement_test import run_all_tests
