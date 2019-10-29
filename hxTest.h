#pragma once
// Copyright 2017 Adrian Johnston
// Copyright 2017 Leap Motion

#include "hatchling.h"
#include "hxProfiler.h"

// Enable this to use GoogleTest
#if HX_GOOGLE_TEST
#include <gtest/gtest.h>
#else // !HX_GOOGLE_TEST

namespace testing {
	class test {
		virtual void hxTest_Execute(void) = 0;
	};
} // namespace testing

// ----------------------------------------------------------------------------
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
		virtual void hxTest_ConstructAndExecute(void) = 0;
		virtual const char* hxTest_ClassName(void) = 0;
		virtual const char* hxTest_FunctionName(void) = 0;
		virtual const char* hxTest_File(void) = 0;
		virtual int32_t hxTest_Line(void) = 0;
	};

	static hxTestRunner& get();

	hxTestRunner() {
		mNumFactories = 0;
		mCurrentTest = 0;
		mFilterClassName = 0;
	}

	void setFilterStaticString(const char* className) { mFilterClassName = className; }
	void addTest(FactoryBase* fn) {
		hxAssertRelease(mNumFactories < MAX_TESTS, "MAX_TESTS overflow\n");
		mFactories[mNumFactories++] = fn;
	}

	// format is required to end with an \n.
	void assertImpl(const char* file, int32_t line, bool condition, const char* format, ...) {
		mTestState = (condition && mTestState != TEST_FAIL) ? TEST_PASS : TEST_FAIL;
		if (!condition) {
			if(++mAssertFailCount >= MAX_FAIL_MESSAGES) {
				if (mAssertFailCount == MAX_FAIL_MESSAGES) {
					hxConsolePrint("Remaining asserts will fail silently...\n");
				}
				return;
			}

			hxConsolePrint("%s.%s", mCurrentTest->hxTest_ClassName(), mCurrentTest->hxTest_FunctionName());
			hxConsolePrint("%s(%d): ", file, (int)line);

			va_list args;
			va_start(args, format);
			hxLogHandlerV(hxLogLevel_Console, format, args);
			va_end(args);
		}
	}

	bool executeAllTests() {
		hxWarnCheck((HX_RELEASE) <= 0, "Running tests with HX_RELEASE > 0");
		hxProfilerInit();

		mPassCount = mFailCount = 0;
		hxLogRelease("hxTestRunner: %s...\n", (mFilterClassName ? mFilterClassName : "All"));
		hxLogRelease("--------\n");
		for (FactoryBase** it = mFactories; it != (mFactories + mNumFactories); ++it) {
			if (!mFilterClassName || ::strcmp(mFilterClassName, (*it)->hxTest_ClassName()) == 0) {
				hxLogRelease("%s.%s...\n", (*it)->hxTest_ClassName(), (*it)->hxTest_FunctionName());

				mTestState = TEST_NOTHING_ASSERTED;
				mAssertFailCount = 0;
				mCurrentTest = *it;

				{
					hxProfileScope((*it)->hxTest_FunctionName());
					// Tests should have no side effects.  Therefore all allocations must be safe to reset.
					hxMemoryManagerScope testTempScope(hxMemoryManagerId_TemporaryStack);
					(*it)->hxTest_ConstructAndExecute();
				}

				if (mTestState == TEST_NOTHING_ASSERTED) {
					assertImpl(hxBasename((*it)->hxTest_File()), (*it)->hxTest_Line(), false, "Nothing was asserted!");
				}
				if (mTestState == TEST_PASS) {
					++mPassCount;
				}
				else {
					++mFailCount;
				}

				hxProfilerLog();
			}
			else {
				hxLogRelease("Skipping %s.%s..\n", (*it)->hxTest_ClassName(), (*it)->hxTest_FunctionName());
			}
		}
		hxLogRelease("--------\n");
		hxProfilerShutdown();
		if (mPassCount > 0 && mFailCount == 0) {
			hxLogHandler(hxLogLevel_Console, "TESTS_PASSED: All %d tests successful.\n", (int)mPassCount);
			return true;
		}
		else {
			hxLogHandler(hxLogLevel_Console, "TEST_FAILED: %d tests failed out of %d.\n", (int)mFailCount, (int)(mFailCount + mPassCount));
			return false;
		}
	}

private:
	hxTestRunner(const hxTestRunner&); // = delete
	void operator=(const hxTestRunner&); // = delete

	FactoryBase* mFactories[MAX_TESTS];
	int32_t mNumFactories;
	TestState mTestState;
	int32_t mAssertFailCount;
	int32_t mPassCount;
	int32_t mFailCount;
	FactoryBase* mCurrentTest;
	const char* mFilterClassName;
};

// ----------------------------------------------------------------------------
// TEST_F.  GoogleTest reimplementation, TestClassName must subclass testing::test.

#define TEST_F(TestClassName, TestFunctionName) \
	struct HX_CONCATENATE(TestClassName, TestFunctionName) : hxTestRunner::FactoryBase { \
		struct TestExecutor : TestClassName { virtual void hxTest_Execute(); }; \
		HX_CONCATENATE(TestClassName, TestFunctionName)() { hxTestRunner::get().addTest(this);  } \
		virtual void hxTest_ConstructAndExecute() { TestExecutor executor; executor.hxTest_Execute(); } \
		virtual const char* hxTest_ClassName() { return #TestClassName; } \
		virtual const char* hxTest_FunctionName() { return #TestFunctionName; } \
		virtual const char* hxTest_File() { return __FILE__; } \
		virtual int32_t hxTest_Line() { return __LINE__; } \
	}; \
	static HX_CONCATENATE(TestClassName, TestFunctionName) HX_CONCATENATE(s_hxTest_, TestFunctionName); \
	void HX_CONCATENATE(TestClassName, TestFunctionName)::TestExecutor::hxTest_Execute(void) // { test code follows:

// ----------------------------------------------------------------------------
// Some tests depend on side effects.  Args must be evaluated exactly once.

#define ASSERT_TRUE(a) hxTestRunner::get().assertImpl(__FILE__, __LINE__, (a), #a"\n")
#define ASSERT_FALSE(a) hxTestRunner::get().assertImpl(__FILE__, __LINE__, !(a), "!("#a")\n")
#define ASSERT_NEAR(a, b, c) hxTestRunner::get().assertImpl(__FILE__, __LINE__, hxAbs((a)-(b)) <= (c), "abs("#a" - "#b") <= "#c"\n")
#define ASSERT_EQ(a, b) hxTestRunner::get().assertImpl(__FILE__, __LINE__, (a) == (b), #a" == "#b"\n")
#define ASSERT_LE(a, b) hxTestRunner::get().assertImpl(__FILE__, __LINE__, (a) <= (b), #a" <= "#b"\n")
#define ASSERT_GE(a, b) hxTestRunner::get().assertImpl(__FILE__, __LINE__, (a) >= (b), #a" >= "#b"\n")

#endif // !HX_GOOGLE_TEST

