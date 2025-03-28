#pragma once
// Copyright 2017-2019 Adrian Johnston
// Copyright 2017 Leap Motion

#include <hx/hatchling.h>

struct hxTestRunner {
public:
	enum TestState {
		TEST_STATE_NOTHING_ASSERTED,
		TEST_STATE_PASS,
		TEST_STATE_FAIL
	};

	enum {
		TEST_MAX_FAIL_MESSAGES = 5,
#if !defined(TEST_MAX_CASES)
		TEST_MAX_CASES = 256
#endif
	};

	struct TestFactoryBase {
		virtual void Run() = 0;
		virtual const char* Suite() = 0;
		virtual const char* Case() = 0;
		virtual const char* File() = 0;
		virtual int32_t Line() = 0;
	};

	// Ensures constructor runs before tests are registered by global constructors.
	static hxTestRunner& singleton() { static hxTestRunner s_hxTestRunner; return s_hxTestRunner; }

	hxTestRunner() {
		mNumFactories = 0;
		mCurrentTest = hxnull;
		mSearchTermStringLiteral = hxnull;
		::memset(mFactories + 0, 0x00, sizeof mFactories);
	}

	void setSearchTerm(const char* searchTermStringLiteral_) { mSearchTermStringLiteral = searchTermStringLiteral_; }

	void addTest(TestFactoryBase* fn_) {
		hxAssertRelease(mNumFactories < TEST_MAX_CASES, "TEST_MAX_CASES overflow\n");
		mFactories[mNumFactories++] = fn_;
	}

	// message is required to end with an \n.  Returns equivalent of /dev/null on
	// success and the system log otherwise.
	hxFile& assertCheck(const char* file_, int32_t line_, bool condition_, const char* message_) {
		++mAssertCount;
		mTestState = (condition_ && mTestState != TEST_STATE_FAIL) ? TEST_STATE_PASS : TEST_STATE_FAIL;
		if (!condition_) {
			if(++mAssertFailCount >= TEST_MAX_FAIL_MESSAGES) {
				if (mAssertFailCount == TEST_MAX_FAIL_MESSAGES) {
					hxLogConsole("remaining asserts will fail silently...\n");
				}
				return devNull_();
			}

			hxLogHandler(hxLogLevel_Console, "%s(%d): ", file_, (int)line_);
			hxLogHandler(hxLogLevel_Console, "%s\n", message_);

			hxAssertRelease(mCurrentTest, "not testing");
			hxLogHandler(hxLogLevel_Assert, "%s.%s", mCurrentTest->Suite(), mCurrentTest->Case());

			return hxout;
		}
		return devNull_();
	}

	int32_t executeAllTests() {
		mPassCount = mFailCount = mAssertCount = 0;
		hxLogConsole("RUNNING_TESTS (%s)\n", (mSearchTermStringLiteral ? mSearchTermStringLiteral : "ALL"));
		for (TestFactoryBase** it_ = mFactories; it_ != (mFactories + mNumFactories); ++it_) {
			if (!mSearchTermStringLiteral || ::strstr(mSearchTermStringLiteral, (*it_)->Suite()) != hxnull) {
				hxLogConsole("%s.%s...\n", (*it_)->Suite(), (*it_)->Case());

				mCurrentTest = *it_;
				mTestState = TEST_STATE_NOTHING_ASSERTED;
				mAssertFailCount = 0;

				{
					// Tests should have no side effects.  Therefore all allocations must be
					// safe to reset.
					hxMemoryManagerScope temporaryStack(hxMemoryManagerId_TemporaryStack);
					(*it_)->Run();
				}

				if (mTestState == TEST_STATE_NOTHING_ASSERTED) {
					assertCheck(hxBasename((*it_)->File()), (*it_)->Line(), false,
						"NOTHING_ASSERTED");
					++mFailCount;
				}
				else if (mTestState == TEST_STATE_PASS) {
					++mPassCount;
				}
				else {
					++mFailCount;
				}
			}
		}

		hxLogConsole("skipped %d tests.  checked %d assertions.\n",
			(int)(mNumFactories - mPassCount - mFailCount), mAssertCount);

		hxWarnCheck(mPassCount + mFailCount, "NOTHING TESTED");

		if (mPassCount != 0 && mFailCount == 0) {
			hxLogHandler(hxLogLevel_Console, "[  PASSED  ] %d test%s.\n", (int)mPassCount,
				((mPassCount != 1) ? "s" : ""));
		}
		else {
			hxLogHandler(hxLogLevel_Console, " %d FAILED TEST%s\n", (int)mFailCount,
				((mFailCount != 1) ? "S" : ""));
			mFailCount = hxmax(mFailCount, 1); // Nothing tested is failure.
		}
		return mFailCount;
	}

private:
	hxFile& devNull_() {
		static hxFile f_(hxFile::out | hxFile::fallible); // Allows writes to fail.
		return f_;
	}

	hxTestRunner(const hxTestRunner&); // = delete
	void operator=(const hxTestRunner&); // = delete

	TestFactoryBase* mFactories[TEST_MAX_CASES];
	int32_t mNumFactories;
	TestFactoryBase* mCurrentTest;
	TestState mTestState;
	int32_t mPassCount;
	int32_t mFailCount;
	const char* mSearchTermStringLiteral;
	int32_t mAssertCount;
	int32_t mAssertFailCount;
};
