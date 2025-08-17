// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.
#pragma once

#include "entanglement_test_a.hpp"

namespace NameSpaceTwo {
	class ENTANGLEMENT_T NameSpaceTwoClassOne {
	public:
		ENTANGLEMENT int class_two_one(int);
		NameSpaceOne::NameSpaceOneClassTwo one_two;
	};
	ENTANGLEMENT int namespace_two(int);
};

namespace NameSpaceOne {
	// Add subclass of a different namespace to a re-opened namespace.
	class ENTANGLEMENT_T NameSpaceOneClassThree : NameSpaceTwo::NameSpaceTwoClassOne {
	public:
		ENTANGLEMENT EnumScopedUInt64 class_one_three(EnumScopedUInt64);
		int pad3;
	};
	// Add overload to re-opened namespace. Important for template programming.
	ENTANGLEMENT int namespace_one(int,int);
};
