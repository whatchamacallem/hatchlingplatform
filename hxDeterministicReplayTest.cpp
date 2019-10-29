// Copyright 2017 Adrian Johnston
// Copyright 2017 Leap Motion

#include "hxDeterministicReplay.h"
#include "hxTest.h"
#include "hxTestPRNG.h"

// ----------------------------------------------------------------------------
#if HX_DETERMINISTIC_REPLAY

static const int32_t s_hxTestFiles = 3;
static const char s_hxTestData[] = "This is a test.";
static const char* s_hxFilename = "DeterministicReplayTest_%d.bin";

class hxDeterministicReplayTest :
	public testing::test
{
public:

	// Either records or checks that recorded data matches:
	static void sharedCodeSection() {
		hxDetermineLabel("label_3");

		const int32_t nums[3] = { 7, 13, 17 };
		hxDetermineData(nums, sizeof nums);

		hxDetermineLabel("label_77");

		hxTestPRNG rng;
		for (int32_t i = 10; i--;) {
			hxDetermineNumber(rng());
		}
	}
};

// ----------------------------------------------------------------------------

TEST_F(hxDeterministicReplayTest, Record) {
	hxDetermine::get().reset(); // Testing only

	for (int32_t i = 0; i < s_hxTestFiles; ++i) {
		const bool isRunning = hxDetermineTick(s_hxFilename, false, 0, s_hxTestFiles);
		ASSERT_TRUE(isRunning);

		// Captures data when recording, use copy for testing.
		char buf[sizeof s_hxTestData];
		::strncpy(buf, s_hxTestData, sizeof s_hxTestData);
		hxDetermineInput(buf, sizeof s_hxTestData);
		ASSERT_TRUE(::strncmp(buf, s_hxTestData, sizeof s_hxTestData) == 0);

		sharedCodeSection();
	}

	const bool isRunning = hxDetermineTick(s_hxFilename, false, 0, s_hxTestFiles);
	ASSERT_FALSE(isRunning); (void)isRunning;
}

TEST_F(hxDeterministicReplayTest, Replay) {
	hxDetermine::get().reset(); // Testing only

	for (int32_t i = 0; i < s_hxTestFiles; ++i) {
		const bool isRunning = hxDetermineTick(s_hxFilename, true, 0, s_hxTestFiles);
		ASSERT_TRUE(isRunning); (void)isRunning;

		// Restores saved data when playing back:
		char buf[256];
		hxDetermineInput(buf, sizeof s_hxTestData);
		ASSERT_TRUE(0 == ::strcmp(buf, s_hxTestData));

		sharedCodeSection();
	}

	const bool isRunning = hxDetermineTick(s_hxFilename, false, 0, s_hxTestFiles);
	ASSERT_FALSE(isRunning); (void)isRunning;
}

#endif // (HX_DETERMINISTIC_REPLAY == 1)


