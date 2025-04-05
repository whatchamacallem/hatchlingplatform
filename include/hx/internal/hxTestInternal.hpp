#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>
#include <hx/hxFile.hpp>

HX_STATIC_ASSERT(!HX_USE_GOOGLE_TEST, "Do not include directly");

struct hxTestFactoryBase_ {
	virtual void Run_() = 0;
	virtual const char* Suite_() = 0;
	virtual const char* Case_() = 0;
	virtual const char* File_() = 0;
	virtual size_t Line_() = 0;
};

struct hxTestRunner_ {
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

	// Ensures constructor runs before tests are registered by global constructors.
	static hxTestRunner_& singleton_() { static hxTestRunner_ s_hxTestRunner; return s_hxTestRunner; }

	hxTestRunner_() {
		m_numFactories = 0;
		m_currentTest = hxnull;
		m_searchTermStringLiteral = hxnull;
		::memset(m_factories + 0, 0x00, sizeof m_factories);
	}

	void setSearchTerm_(const char* searchTermStringLiteral_) { m_searchTermStringLiteral = searchTermStringLiteral_; }

	void addTest_(hxTestFactoryBase_* fn_) {
		hxAssertRelease(m_numFactories < TEST_MAX_CASES, "TEST_MAX_CASES overflow\n");
		m_factories[m_numFactories++] = fn_;
	}

	// message is required to end with an \n.  Returns equivalent of /dev/null on
	// success and the system log otherwise.
	hxFile& assertCheck_(const char* file_, size_t line_, bool condition_, const char* message_) {
		++m_assertCount;
		m_testState = (condition_ && m_testState != TEST_STATE_FAIL) ? TEST_STATE_PASS : TEST_STATE_FAIL;
		if (!condition_) {
			if(++m_assertFailCount >= TEST_MAX_FAIL_MESSAGES) {
				if (m_assertFailCount == TEST_MAX_FAIL_MESSAGES) {
					hxLogConsole("remaining asserts will fail silently...\n");
				}
				return devNull_();
			}

			hxLogHandler(hxLogLevel_Console, "%s(%zu): ", file_, line_);
			hxLogHandler(hxLogLevel_Console, "%s\n", message_);

			hxAssertRelease(m_currentTest, "not testing");
			hxLogHandler(hxLogLevel_Assert, "%s.%s", m_currentTest->Suite_(), m_currentTest->Case_());

			return devNull_();
		}
		return devNull_();
	}

	size_t executeAllTests_() {
		m_passCount = m_failCount = m_assertCount = 0;
		hxLogConsole("RUNNING_TESTS (%s)\n", (m_searchTermStringLiteral ? m_searchTermStringLiteral : "ALL"));
		for (hxTestFactoryBase_** it_ = m_factories; it_ != (m_factories + m_numFactories); ++it_) {
			if (!m_searchTermStringLiteral || ::strstr(m_searchTermStringLiteral, (*it_)->Suite_()) != hxnull) {
				hxLogConsole("%s.%s...\n", (*it_)->Suite_(), (*it_)->Case_());

				m_currentTest = *it_;
				m_testState = TEST_STATE_NOTHING_ASSERTED;
				m_assertFailCount = 0;

				{
					// Tests should have no side effects.  Therefore all allocations must be
					// safe to reset.
					hxMemoryManagerScope temporaryStack(hxMemoryManagerId_TemporaryStack);
					(*it_)->Run_();
				}

				if (m_testState == TEST_STATE_NOTHING_ASSERTED) {
					assertCheck_(hxBasename((*it_)->File_()), (*it_)->Line_(), false,
						"NOTHING_ASSERTED");
					++m_failCount;
				}
				else if (m_testState == TEST_STATE_PASS) {
					++m_passCount;
				}
				else {
					++m_failCount;
				}
			}
		}

		hxLogConsole("skipped %zu tests.  checked %zu assertions.\n",
			m_numFactories - m_passCount - m_failCount, m_assertCount);

		hxWarnCheck(m_passCount + m_failCount, "NOTHING TESTED");

		if (m_passCount != 0 && m_failCount == 0) {
			hxLogHandler(hxLogLevel_Console, "[  PASSED  ] %zu test%s.\n", m_passCount,
				((m_passCount != 1) ? "s" : ""));
		}
		else {
			hxLogHandler(hxLogLevel_Console, " %zu FAILED TEST%s\n", m_failCount,
				((m_failCount != 1) ? "S" : ""));
			m_failCount = hxmax(m_failCount, (size_t)1u); // Nothing tested is failure.
		}
		return m_failCount;
	}

private:
	hxFile& devNull_() {
		static hxFile f_(hxFile::out | hxFile::fallible); // Allows writes to fail.
		return f_;
	}

	hxTestRunner_(const hxTestRunner_&); // = delete
	void operator=(const hxTestRunner_&); // = delete

	hxTestFactoryBase_* m_factories[TEST_MAX_CASES];
	size_t m_numFactories;
	hxTestFactoryBase_* m_currentTest;
	TestState m_testState;
	size_t m_passCount;
	size_t m_failCount;
	const char* m_searchTermStringLiteral;
	size_t m_assertCount;
	size_t m_assertFailCount;
};
