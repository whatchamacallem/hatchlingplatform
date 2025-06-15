#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>
#include <hx/hxFile.hpp>

HX_STATIC_ASSERT(!HX_USE_GOOGLE_TEST, "Do not include directly");

struct hxTestCaseBase_ {
	virtual void run_() = 0;
	virtual const char* suite_() = 0;
	virtual const char* case_() = 0;
	virtual const char* file_() = 0;
	virtual size_t line_() = 0;
};

struct hxTestSuiteExecutor_ {
public:
	enum TestState_ {
		TEST_STATE_NOTHING_ASSERTED_,
		TEST_STATE_PASS_,
		TEST_STATE_FAIL_
	};

	enum {
		TEST_MAX_FAIL_MESSAGES_ = 5,
#if !defined(HX_TEST_MAX_CASES)
		HX_TEST_MAX_CASES = 1024
#endif
	};

	// Ensures constructor runs before tests are registered by global constructors.
	static hxTestSuiteExecutor_& singleton_() { static hxTestSuiteExecutor_ s_hxTestRunner; return s_hxTestRunner; }

	hxTestSuiteExecutor_() {
		m_searchTermStringLiteral_ = hxnull;
		m_numFactories_ = 0;
		m_currentTest_ = hxnull;
		::memset(m_factories_ + 0, 0x00, sizeof m_factories_);
	}

	void setSearchTerm_(const char* searchTermStringLiteral_) {
		m_searchTermStringLiteral_ = searchTermStringLiteral_;
	}

	void addTest_(hxTestCaseBase_* fn_) {
		// Use -DHX_TEST_MAX_CASES to provide enough room for all tests.
		hxAssertRelease(m_numFactories_ < HX_TEST_MAX_CASES, "HX_TEST_MAX_CASES overflow\n");
		if(m_numFactories_ < HX_TEST_MAX_CASES) {
			m_factories_[m_numFactories_++] = fn_;
		}
	}

	// message is required to end with an \n. Returns equivalent of /dev/null on
	// success and the system log otherwise.
	hxFile& assertCheck_(const char* file_, size_t line_, bool condition_, const char* message_) {
		hxAssertRelease(m_currentTest_, "not testing");
		++m_assertCount_;
		m_testState_ = (condition_ && m_testState_ != TEST_STATE_FAIL_) ? TEST_STATE_PASS_ : TEST_STATE_FAIL_;
		if (!condition_) {
			if(++m_assertFailCount_ >= TEST_MAX_FAIL_MESSAGES_) {
				if (m_assertFailCount_ == TEST_MAX_FAIL_MESSAGES_) {
					hxLogConsole("remaining asserts will fail silently...\n");
				}
				return fileNull_();
			}

			// prints full path error messages that can be clicked on in an ide.
			hxLogHandler(hxLogLevel_Assert, "%s.%s", m_currentTest_->suite_(), m_currentTest_->case_());
			hxLogHandler(hxLogLevel_Assert, "%s(%zu): %s", file_, line_, message_);

			// Implements GTEST_FLAG_SET(break_on_failure, true);
#if (HX_TEST_ERROR_HANDLING) == 0 && (HX_RELEASE) == 0
			HX_BREAKPOINT();
#endif
			return fileLog_();
		}
		return fileNull_();
	}

	size_t executeAllTests_() {
		hxInit(); // RUN_ALL_TESTS could be called first.
		m_passCount_ = m_failCount_ = m_assertCount_ = 0;
		hxLogConsole("RUNNING_TESTS (%s)\n", (m_searchTermStringLiteral_ ? m_searchTermStringLiteral_ : "ALL"));
		for (hxTestCaseBase_** it_ = m_factories_; it_ != (m_factories_ + m_numFactories_); ++it_) {
			if (!m_searchTermStringLiteral_ || ::strstr(m_searchTermStringLiteral_, (*it_)->suite_()) != hxnull) {
				hxLogConsole("%s.%s...\n", (*it_)->suite_(), (*it_)->case_());

				m_currentTest_ = *it_;
				m_testState_ = TEST_STATE_NOTHING_ASSERTED_;
				m_assertFailCount_ = 0;

#ifdef __cpp_exceptions
				try
#endif
				{
					// Tests should have no side effects. Therefore all allocations must be
					// safe to reset.
					hxMemoryAllocatorScope temporaryStack(hxMemoryAllocator_TemporaryStack);
					(*it_)->run_();
				}
#ifdef __cpp_exceptions
				catch (...) {
					this->assertCheck_((*it_)->file_(), (*it_)->line_(), false, "unexpected exception");
				}
#endif

				if (m_testState_ == TEST_STATE_NOTHING_ASSERTED_) {
					this->assertCheck_((*it_)->file_(), (*it_)->line_(), false, "NOTHING_ASSERTED");
					++m_failCount_;
				}
				else if (m_testState_ == TEST_STATE_PASS_) {
					++m_passCount_;
				}
				else {
					++m_failCount_;
				}
			}
		}

		hxLogConsole("skipped %zu tests. checked %zu assertions.\n",
			m_numFactories_ - m_passCount_ - m_failCount_, m_assertCount_);

		hxWarnMsg(m_passCount_ + m_failCount_, "NOTHING TESTED");

		if (m_passCount_ != 0 && m_failCount_ == 0) {
			hxLogHandler(hxLogLevel_Console, "[  PASSED  ] %zu test%s.\n", m_passCount_,
				((m_passCount_ != 1) ? "s" : ""));
		}
		else {
			hxLogHandler(hxLogLevel_Console, " %zu FAILED TEST%s\n", m_failCount_,
				((m_failCount_ != 1) ? "S" : ""));
			m_failCount_ = hxmax(m_failCount_, (size_t)1u); // Nothing tested is failure.
		}
		return m_failCount_;
	}

private:
	hxFile& fileNull_() { static hxFile f_(hxFile::out | hxFile::failable); return f_; }
	hxFile& fileLog_()  { static hxFile f_(hxFile::out | hxFile::stdio); return f_; }

	hxTestSuiteExecutor_(const hxTestSuiteExecutor_&) HX_DELETE_FN;
	void operator=(const hxTestSuiteExecutor_&) HX_DELETE_FN;

	const char* m_searchTermStringLiteral_;
	hxTestCaseBase_* m_factories_[HX_TEST_MAX_CASES];
	size_t m_numFactories_;
	hxTestCaseBase_* m_currentTest_;
	TestState_ m_testState_;
	size_t m_passCount_;
	size_t m_failCount_;
	size_t m_assertCount_;
	size_t m_assertFailCount_;
};
