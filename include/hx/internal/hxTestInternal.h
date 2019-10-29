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
		virtual void Run(void) = 0;
		virtual const char* Suite(void) = 0;
		virtual const char* Case(void) = 0;
		virtual const char* File(void) = 0;
		virtual int32_t Line(void) = 0;
	};

	// Ensures constructor runs before tests are registered by global constructors.
	static hxTestRunner& get() { static hxTestRunner s_hxTestRunner; return s_hxTestRunner; }

	hxTestRunner() {
		mNumFactories = 0;
		mCurrentTest = 0;
		mFilterSuiteName = hxnull;
		::memset(mFactories + 0, 0x00, sizeof mFactories);
	}

	void setFilterStringLiteral(const char* className) { mFilterSuiteName = className; }

	void addTest(FactoryBase* fn) {
		hxAssertRelease(mNumFactories < MAX_TESTS, "MAX_TESTS overflow\n");
		mFactories[mNumFactories++] = fn;
	}

	// format is required to end with an \n.  Returns /dev/null on success and the system log otherwise.
	hxFile& assertCheck(const char* file, int32_t line, bool condition, const char* format, ...) {
		mTestState = (condition && mTestState != TEST_FAIL) ? TEST_PASS : TEST_FAIL;
		if (!condition) {
			if(++mAssertFailCount >= MAX_FAIL_MESSAGES) {
				if (mAssertFailCount == MAX_FAIL_MESSAGES) {
					hxLogConsole("Remaining asserts will fail silently...\n");
				}
				return devNull();
			}

			hxLogHandler(hxLogLevel_Assert, "%s.%s", mCurrentTest->Suite(), mCurrentTest->Case());
			hxLogConsole("%s(%d): ", file, (int)line);

			va_list args;
			va_start(args, format);
			hxLogHandlerV(hxLogLevel_Console, format, args);
			va_end(args);

			return hxLogFile();
		}
		return devNull();
	}

	bool executeAllTests() {
		hxWarnCheck((HX_RELEASE) <= 0, "Running tests with HX_RELEASE > 0");

		mPassCount = mFailCount = 0;
		hxLogConsole("RUNNING_TESTS (%s)\n", (mFilterSuiteName ? mFilterSuiteName : "ALL"));
		for (FactoryBase** it = mFactories; it != (mFactories + mNumFactories); ++it) {
			if (!mFilterSuiteName || ::strcmp(mFilterSuiteName, (*it)->Suite()) == 0) {
				hxLogConsole("%s.%s...\n", (*it)->Suite(), (*it)->Case());

				mTestState = TEST_NOTHING_ASSERTED;
				mAssertFailCount = 0;
				mCurrentTest = *it;

				{
					// Tests should have no side effects.  Therefore all allocations must be safe to reset.
					hxMemoryManagerScope temporaryStack(hxMemoryManagerId_TemporaryStack);
					(*it)->Run();
				}

				if (mTestState == TEST_NOTHING_ASSERTED) {
					assertCheck(hxBasename((*it)->File()), (*it)->Line(), false, "NOTHING ASSERTED");
				}
				else if (mTestState == TEST_PASS) {
					++mPassCount;
				}
				else {
					++mFailCount;
				}
			}
			else {
				hxLogConsole("Skipping %s.%s..\n", (*it)->Suite(), (*it)->Case());
			}
		}
		hxProfilerStop();
		if (mPassCount > 0 && mFailCount == 0) {
			hxLogHandler(hxLogLevel_Console, "[  PASSED  ] %d tests.\n", (int)mPassCount);
			return true;
		}
		else {
			hxLogHandler(hxLogLevel_Console, " %d FAILED TEST%s\n", (int)mFailCount, ((mFailCount > 1) ? "S" : ""));
			return false;
		}
	}

private:
	hxFile& devNull() {
		static hxFile f(hxFile::out | hxFile::fallible);
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
	const char* mFilterSuiteName;
	int32_t mAssertFailCount;
};
