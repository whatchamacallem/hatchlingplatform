// Copyright 2017-2019 Adrian Johnston
// Copyright 2017 Leap Motion

#include <hx/hatchling.h>
#include <hx/hxTest.h>

HX_REGISTER_FILENAME_HASH

// ----------------------------------------------------------------------------

TEST(hxMemoryManagerTest, ZeroBytes) {
	void* p = hxMalloc(0u);
	ASSERT_TRUE(p != hxnull);
	hxFree(p);
}

TEST(hxMemoryManagerTest, StringDuplicateNull) {
	// duplicating a null string is null.
	void* p = hxStringDuplicate(hxnull);
	ASSERT_TRUE(p == hxnull);
	hxFree(hxnull);
}

TEST(hxMemoryManagerTest, StringDuplicate) {
	char* p = hxStringDuplicate("str");
	ASSERT_TRUE(p != hxnull);
	ASSERT_TRUE(::strcmp(p, "str") == 0);
	hxFree(p);
}

// ----------------------------------------------------------------------------

#if (HX_MEM_DIAGNOSTIC_LEVEL) != -1

class hxMemoryManagerTest :
	public testing::Test
{
public:
	void TestMemoryAllocatorNormal(hxMemoryManagerId id) {
		uintptr_t startCount;
		uintptr_t startBytes;

		{
			hxLog("id %d...\n", (int)id);
			hxMemoryManagerScope allocatorScope(id);

			startCount = allocatorScope.getTotalAllocationCount();
			startBytes = allocatorScope.getTotalBytesAllocated();

			void* ptr1 = hxMalloc(100);
			void* ptr2 = hxMalloc(200);
			::memset(ptr1, 0x33, 100);
			::memset(ptr2, 0x33, 200);

			{
				// Google Test spams new/delete with std::string operations:
				hxMemoryManagerScope spamGuard(hxMemoryManagerId_Heap);
				ASSERT_EQ(allocatorScope.getScopeAllocationCount(), 2u);
				ASSERT_EQ(allocatorScope.getPreviousAllocationCount(), startCount);
				ASSERT_EQ(allocatorScope.getTotalAllocationCount(), 2u + startCount);
				ASSERT_NEAR(allocatorScope.getScopeBytesAllocated(), 300u, 2u * HX_ALIGNMENT_MASK);
				ASSERT_NEAR(allocatorScope.getTotalBytesAllocated(), startBytes + 300u, 2u * HX_ALIGNMENT_MASK);
				ASSERT_EQ(allocatorScope.getPreviousBytesAllocated(), startBytes);
			}

			// Allow quiet deletion of a resource.
			g_hxSettings.isShuttingDown = true;
			hxFree(ptr1);
			hxFree(ptr2);
			g_hxSettings.isShuttingDown = false;

			// Special case for heaps that do not track free.
			if (allocatorScope.getScopeBytesAllocated() != 0) {
				// Google Test spams new/delete with std::string operations:
				hxMemoryManagerScope spamGuard(hxMemoryManagerId_Heap);

				// The debug heap requires HX_ALLOCATIONS_LOG_LEVEL enabled to track bytes allocated.
				ASSERT_NEAR(allocatorScope.getScopeBytesAllocated(), 300, 2 * HX_ALIGNMENT_MASK);
			}
			else {
				hxMemoryManagerScope spamGuard(hxMemoryManagerId_Heap);
				ASSERT_EQ(allocatorScope.getScopeBytesAllocated(), 0u);
				ASSERT_EQ(allocatorScope.getTotalBytesAllocated(), startBytes);
			}
		}

		// hxMemoryManagerId_Permanent does not free.
		if (id != hxMemoryManagerId_Permanent) {
			hxMemoryManagerScope allocatorScope(id);

			// Google Test spams new/delete with std::string operations:
			hxMemoryManagerScope spamGuard(hxMemoryManagerId_Heap);
			ASSERT_EQ(allocatorScope.getPreviousAllocationCount(), startCount);
			ASSERT_EQ(allocatorScope.getPreviousBytesAllocated(), startBytes);
		}
	}

	void TestMemoryAllocatorLeak(hxMemoryManagerId id) {
		(void)id;
#if (HX_RELEASE) < 1
		uintptr_t startCount = 0;
		uintptr_t startBytes = 0;
		void* ptr2 = hxnull;
		int32_t assertsAllowed = g_hxSettings.assertsToBeSkipped;

		{
			hxMemoryManagerScope allocatorScope(id);

			startCount = allocatorScope.getScopeAllocationCount();
			startBytes = allocatorScope.getScopeBytesAllocated();

			void* ptr1 = hxMalloc(100);
			ptr2 = hxMalloc(200);
			::memset(ptr1, 0x33, 100);
			::memset(ptr2, 0x33, 200);

			hxFree(ptr1); // Only free the one.

			g_hxSettings.assertsToBeSkipped = 1;
		}
		ASSERT_EQ(g_hxSettings.assertsToBeSkipped, 0); // hxAssert was hit, leak in scope

		hxMemoryManagerScope allocatorScope(id);
		{
			// Google Test spams new/delete with std::string operations:
			hxMemoryManagerScope spamGuard(hxMemoryManagerId_Heap);
			ASSERT_EQ(allocatorScope.getPreviousAllocationCount(), startCount);
			ASSERT_EQ(allocatorScope.getPreviousBytesAllocated(), startBytes);
		}

		g_hxSettings.assertsToBeSkipped = 1;
		hxFree(ptr2);
		ASSERT_EQ(g_hxSettings.assertsToBeSkipped, 0); // hxAssert was hit, free after scope closed
		g_hxSettings.assertsToBeSkipped = assertsAllowed;
#endif
	}
};

TEST_F(hxMemoryManagerTest, Execute) {
#if (HX_MEM_DIAGNOSTIC_LEVEL) >= 1
	if (g_hxSettings.disableMemoryManager) {
		return; // Test fails because the hxMemoryManager code is disabled.
	}
#endif
	hxLog("TEST_EXPECTING_ASSERTS:\n");

	for (int32_t i = 0; i < hxMemoryManagerId_MAX; ++i) {
		TestMemoryAllocatorNormal((hxMemoryManagerId)i);
	}

	// Only the TemporaryStack expects all allocations to be free()'d.
	TestMemoryAllocatorLeak(hxMemoryManagerId_TemporaryStack);
}

TEST(hxMemoryManagerTest, TempOverflow) {
	// there is no policy against using the debug heap in release
	void* p = hxMallocExt(HX_MEMORY_BUDGET_TEMPORARY_STACK + 1, hxMemoryManagerId_TemporaryStack, 0u);
	ASSERT_TRUE(p != hxnull);
	ASSERT_TRUE(!hxIsScratchpad(p)); // both overflowed, not yet on scratchpad.
	hxFree(p);

	hxMemoryManagerScope temp(hxMemoryManagerId_TemporaryStack);
	p = hxMalloc(HX_MEMORY_BUDGET_TEMPORARY_STACK + 1);
	ASSERT_TRUE(p != hxnull);
	hxFree(p);
}

#if HX_USE_MEMORY_SCRATCH
TEST(hxMemoryManagerTest, ScratchOverflow) {
	hxMemoryManagerScope temp(hxMemoryManagerId_ScratchTemp);

	// exercise ext version.
	// there is no policy against using the debug heap in release
	void* p = hxMallocExt(HX_MEMORY_BUDGET_SCRATCH_TEMP + 1, hxMemoryManagerId_ScratchTemp, 0u);
	ASSERT_TRUE(p != hxnull);
	ASSERT_TRUE(!hxIsScratchpad(p)); // overflowed
	hxFree(p);
}
#endif // HX_USE_MEMORY_SCRATCH

#endif // HX_MEM_DIAGNOSTIC_LEVEL == -1
