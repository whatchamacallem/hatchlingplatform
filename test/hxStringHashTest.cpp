// Copyright 2017-2019 Adrian Johnston

#include <hx/hatchling.h>
#include <hx/hxTest.h>

HX_REGISTER_FILENAME_HASH

// ----------------------------------------------------------------------------
TEST(hxStringHashTest, Equality) {
	uint32_t hash1 = hxStringLiteralHash("");
	uint32_t hash2 = hxStringLiteralHash("abc");
	uint32_t hash3 = hxStringLiteralHash(
		"The quick brown fox jumps over the lazy dog. "
		"1234567890qwertyuiopasdfghjklzxcvbnm"
		"1234567890qwertyuiopasdfghjklzxcvbnm"
		"1234567890qwertyuiopasdfghjklzxcvbnm"
		"1234567890qwertyuiopasdfghjklzxcvbnm"
		"123456");

	ASSERT_EQ(hash1, hxStringLiteralHashDebug(""));
	ASSERT_EQ(hash2, hxStringLiteralHashDebug("abc"));
	ASSERT_EQ(hash3, hxStringLiteralHashDebug(
		"The quick brown fox jumps over the lazy dog. "
		"1234567890qwertyuiopasdfghjklzxcvbnm"
		"1234567890qwertyuiopasdfghjklzxcvbnm"
		"1234567890qwertyuiopasdfghjklzxcvbnm"
		"1234567890qwertyuiopasdfghjklzxcvbnm"
		"123456"));
}
