// Copyright 2017 Adrian Johnston

#include "hatchling.h"
#include "hxTest.h"

HX_REGISTER_FILENAME_HASH

// ----------------------------------------------------------------------------
class hxStringHashTest :
	public testing::test
{
};

// ----------------------------------------------------------------------------
TEST_F(hxStringHashTest, Equality) {
	uint32_t hash1 = hxHashStringLiteral("");
	uint32_t hash2 = hxHashStringLiteral("abc");
	uint32_t hash3 = hxHashStringLiteral(
		"The quick brown fox jumps over the lazy dog. "
		"1234567890qwertyuiopasdfghjklzxcvbnm"
		"1234567890qwertyuiopasdfghjklzxcvbnm"
		"1234567890qwertyuiopasdfghjklzxcvbnm"
		"1234567890qwertyuiopasdfghjklzxcvbnm"
		"123456");

	ASSERT_EQ(hash1, hxHashStringLiteralDebug(""));
	ASSERT_EQ(hash2, hxHashStringLiteralDebug("abc"));
	ASSERT_EQ(hash3, hxHashStringLiteralDebug(
		"The quick brown fox jumps over the lazy dog. "
		"1234567890qwertyuiopasdfghjklzxcvbnm"
		"1234567890qwertyuiopasdfghjklzxcvbnm"
		"1234567890qwertyuiopasdfghjklzxcvbnm"
		"1234567890qwertyuiopasdfghjklzxcvbnm"
		"123456"));
}
