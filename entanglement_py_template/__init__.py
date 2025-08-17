# SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
# SPDX-License-Identifier: MIT
# This file is licensed under the MIT license found in the LICENSE.md file.

# List what 'from x import *' should include.
__all__ = ['main', 'run_all_tests']

from .entanglement import main
from .entanglement_py_template_test import run_all_tests
