// Copyright 2017-2025 Adrian Johnston
#pragma once

#include <entanglement.h>
#include <stdint.h>

enum ENTANGLEMENT_TYPE {
	ANONYMOUS_ENUM_0
};
enum ENTANGLEMENT_TYPE EnumCStyleTwoConstants {
	ENUM_C_STYLE_TWO_CONSTANTS_1 = 1,
	ENUM_C_STYLE_TWO_CONSTANTS_2
};
enum ENTANGLEMENT_TYPE EnumInt16ThreeConstants : int16_t {
	ENUM_INT16_THREE_CONSTANTS_0 = -32768,
	ENUM_INT16_THREE_CONSTANTS_1 = -1,
	ENUM_INT16_THREE_CONSTANTS_2 = 32767
};
enum class ENTANGLEMENT_TYPE EnumScopedUInt64 : uint64_t {
	ENUM_SCOPED_UINT64_0 = 0xabcdef0123456789
};
