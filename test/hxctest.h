#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include <hx/hatchling.h>

#if HX_CPLUSPLUS
extern "C" {
#endif

bool hxctest_all(void);

bool hxctest_hatchling_h(void);
bool hxctest_math(void);
bool hxctest_clamp(void);
bool hxctest_swap(void);
bool hxctest_memory(void);

#if HX_CPLUSPLUS
} // extern "C"
#endif
