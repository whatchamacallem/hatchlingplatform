#pragma once
// Copyright 2017-2019 Adrian Johnston
// Copyright 2017 Leap Motion

#include <hx/hatchling.h>

struct hxTestRunner {
public:
	enum TestState {
		TEST_NOTHING_ASSERTED,
		TEST_PASS,
		TEST_FAIL
	};

	enum {
		MAX_FAIL_MESSAGES = 5,
		MAX_TESTS = 256
	};

	struct FactoryBase {
		virtual void Run() = 0;
		virtual const char* Suite() = 0;
		virtual const char* Case() = 0;
		virtual const char* File() = 0;
		virtual int32_t Line() = 0;
	};

	// Ensures constructor runs before tests are registered by global constructors.
	static hxTestRunner& get() { static hxTestRunner s_hxTestRunner; return s_hxTestRunner; }

	hxTestRunner() {
		mNumFactories = 0;
		mCurrentTest = hxnull;
		mSearchTermStringLiteral = hxnull;
		::memset(mFactories + 0, 0x00, sizeof mFactories);
	}

	void setSearchTerm(const char* searchTermStringLiteral) { mSearchTermStringLiteral = searchTermStringLiteral; }

	void addTest(FactoryBase* fn) {
		hxAssertRelease(mNumFactories < MAX_TESTS, "MAX_TESTS overflow\n");
		mFactories[mNumFactories++] = fn;
	}

	// format is required to end with an \n.  Returns /dev/null on success and
	// the system log otherwise.
	hxFile& assertCheck(const char* file, int32_t line, bool condition, const char* format, ...) {
		++mAssertCount;
		mTestState = (condition && mTestState != TEST_FAIL) ? TEST_PASS : TEST_FAIL;
		if (!condition) {
			if(++mAssertFailCount >= MAX_FAIL_MESSAGES) {
				if (mAssertFailCount == MAX_FAIL_MESSAGES) {
					hxLogConsole("remaining asserts will fail silently...\n");
				}
				return devNull();
			}

			hxLogConsole("%s(%d): ", file, (int)line); (void)file; (void)line;
			va_list args;
			va_start(args, format);
			hxLogHandlerV(hxLogLevel_Console, format, args);
			va_end(args);

			hxAssertRelease(mCurrentTest, "not testing");
			hxLogHandler(hxLogLevel_Assert, "%s.%s", mCurrentTest->Suite(), mCurrentTest->Case());

			return hxout;
		}
		return devNull();
	}

	int32_t executeAllTests() {
		mPassCount = mFailCount = mAssertCount = 0;
		hxLogConsole("RUNNING_TESTS (%s)\n", (mSearchTermStringLiteral ? mSearchTermStringLiteral : "ALL"));
		for (FactoryBase** it = mFactories; it != (mFactories + mNumFactories); ++it) {
			if (!mSearchTermStringLiteral || ::strstr(mSearchTermStringLiteral, (*it)->Suite()) != hxnull) {
				hxLogConsole("%s.%s...\n", (*it)->Suite(), (*it)->Case());

				mCurrentTest = *it;
				mTestState = TEST_NOTHING_ASSERTED;
				mAssertFailCount = 0;

				{
					// Tests should have no side effects.  Therefore all allocations must be
					// safe to reset.
					hxMemoryManagerScope temporaryStack(hxMemoryManagerId_TemporaryStack);
					(*it)->Run();
				}

				if (mTestState == TEST_NOTHING_ASSERTED) {
					assertCheck(hxBasename((*it)->File()), (*it)->Line(), false,
						"NOTHING ASSERTED\n");
					++mFailCount;
				}
				else if (mTestState == TEST_PASS) {
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
			mFailCount = hxMax(mFailCount, 1); // Nothing tested is failure.
		}
		return mFailCount;
	}

private:
	hxFile& devNull() {
		static hxFile f(hxFile::out | hxFile::fallible); // Allows writes to fail.
		return f;
	}

	hxTestRunner(const hxTestRunner&); // = delete
	void operator=(const hxTestRunner&); // = delete

	FactoryBase* mFactories[MAX_TESTS];
	int32_t mNumFactories;
	FactoryBase* mCurrentTest;
	TestState mTestState;
	int32_t mPassCount;
	int32_t mFailCount;
	const char* mSearchTermStringLiteral;
	int32_t mAssertCount;
	int32_t mAssertFailCount;
};
