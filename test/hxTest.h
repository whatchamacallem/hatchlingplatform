#pragma once
// Copyright 2017-2019 Adrian Johnston
// Copyright 2017 Leap Motion

#include <hx/hatchling.h>
#include <hx/hxProfiler.h>
#include <hx/hxFile.h>

// Enable this to use GoogleTest instead of hxTestRunner.
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
		::memset(mFactories + 0, 0x00, sizeof mFactories);
	}

	void setFilterStaticString(const char* className) { mFilterClassName = className; }
	void addTest(FactoryBase* fn) {
		hxAssertRelease(mNumFactories < MAX_TESTS, "MAX_TESTS overflow\n");
		mFactories[mNumFactories++] = fn;
	}

	// format is required to end with an \n.  Returns /dev/null on success and the system log otherwise.
	hxFile& assertImpl(const char* file, int32_t line, bool condition, const char* format, ...) {
		mTestState = (condition && mTestState != TEST_FAIL) ? TEST_PASS : TEST_FAIL;
		if (!condition) {
			if(++mAssertFailCount >= MAX_FAIL_MESSAGES) {
				if (mAssertFailCount == MAX_FAIL_MESSAGES) {
					hxLogConsole("Remaining asserts will fail silently...\n");
				}
				return devNull();
			}

			hxLogHandler(hxLogLevel_Assert, "%s.%s", mCurrentTest->hxTest_ClassName(), mCurrentTest->hxTest_FunctionName());
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
		hxLogConsole("RUNNING_TESTS (%s)\n", (mFilterClassName ? mFilterClassName : "ALL"));
		for (FactoryBase** it = mFactories; it != (mFactories + mNumFactories); ++it) {
			if (!mFilterClassName || ::strcmp(mFilterClassName, (*it)->hxTest_ClassName()) == 0) {
				hxLogConsole("%s.%s...\n", (*it)->hxTest_ClassName(), (*it)->hxTest_FunctionName());

				mTestState = TEST_NOTHING_ASSERTED;
				mAssertFailCount = 0;
				mCurrentTest = *it;

				{
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
			}
			else {
				hxLogConsole("Skipping %s.%s..\n", (*it)->hxTest_ClassName(), (*it)->hxTest_FunctionName());
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
	TestState mTestState;
	int32_t mAssertFailCount;
	int32_t mPassCount;
	int32_t mFailCount;
	FactoryBase* mCurrentTest;
	const char* mFilterClassName;
};

// ----------------------------------------------------------------------------
// GoogleTest reimplementation.

// TEST.
#define TEST(TestClassName, TestFunctionName) \
	struct HX_CONCATENATE(TestClassName, TestFunctionName) : hxTestRunner::FactoryBase { \
		struct TestExecutor { virtual void hxTest_Execute(); }; \
		HX_CONCATENATE(TestClassName, TestFunctionName)() { hxTestRunner::get().addTest(this);  } \
		virtual void hxTest_ConstructAndExecute() { TestExecutor executor; executor.hxTest_Execute(); } \
		virtual const char* hxTest_ClassName() { return #TestClassName; } \
		virtual const char* hxTest_FunctionName() { return #TestFunctionName; } \
		virtual const char* hxTest_File() { return __FILE__; } \
		virtual int32_t hxTest_Line() { return __LINE__; } \
	}; \
	static HX_CONCATENATE(TestClassName, TestFunctionName) HX_CONCATENATE(s_hxTest_, TestFunctionName); \
	void HX_CONCATENATE(TestClassName, TestFunctionName)::TestExecutor::hxTest_Execute(void) // { test code follows:

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

// Some tests depend on side effects.  Args must be evaluated exactly once.
#define ASSERT_TRUE(x) hxTestRunner::get().assertImpl(__FILE__, __LINE__, (x), #x "\n")
#define ASSERT_FALSE(x) hxTestRunner::get().assertImpl(__FILE__, __LINE__, !(x), "!" #x "\n")
#define ASSERT_NEAR(expected, actual, absolute_range) hxTestRunner::get().assertImpl(__FILE__, __LINE__, hxAbs((expected)-(actual)) <= (absolute_range), "abs(" #expected "-" #actual ")<=" #absolute_range "\n")
#define ASSERT_EQ(lhs, rhs) hxTestRunner::get().assertImpl(__FILE__, __LINE__, (lhs) == (rhs), #lhs "==" #rhs "\n")
#define ASSERT_NE(lhs, rhs) hxTestRunner::get().assertImpl(__FILE__, __LINE__, (lhs) != (rhs), #lhs "!=" #rhs "\n")
#define ASSERT_LE(lhs, rhs) hxTestRunner::get().assertImpl(__FILE__, __LINE__, (lhs) <= (rhs), #lhs "<=" #rhs "\n")
#define ASSERT_GE(lhs, rhs) hxTestRunner::get().assertImpl(__FILE__, __LINE__, (lhs) >= (rhs), #lhs ">=" #rhs "\n")
#define ASSERT_LT(lhs, rhs) hxTestRunner::get().assertImpl(__FILE__, __LINE__, (lhs) < (rhs), #lhs "<" #rhs "\n")
#define ASSERT_GT(lhs, rhs) hxTestRunner::get().assertImpl(__FILE__, __LINE__, (lhs) > (rhs), #lhs ">" #rhs "\n")

#define EXPECT_TRUE ASSERT_TRUE
#define EXPECT_FALSE ASSERT_FALSE
#define EXPECT_NEAR ASSERT_NEAR
#define EXPECT_EQ ASSERT_EQ
#define EXPECT_NE ASSERT_NE
#define EXPECT_LE ASSERT_LE
#define EXPECT_GE ASSERT_GE
#define EXPECT_LT ASSERT_LT
#define EXPECT_GT ASSERT_GT

#endif // !HX_GOOGLE_TEST

