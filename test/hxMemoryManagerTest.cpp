// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>
#include <hx/hxTest.hpp>

HX_REGISTER_FILENAME_HASH

TEST(hxMemoryManagerTestF, Bytes) {
	for(size_t i=0u; i<10u; ++i) {
		void* p = hxMalloc(i);
		ASSERT_TRUE(p != hxnull);
		::memset(p, 0x66, i);
		hxFree(p);
	}
}

TEST(hxMemoryManagerTestF, StringDuplicateNull) {
	// duplicating a null string is null.
	void* p = hxStringDuplicate(hxnull);
	ASSERT_TRUE(p == hxnull);
	hxFree(hxnull);
}

TEST(hxMemoryManagerTestF, StringDuplicate) {
	char* p = hxStringDuplicate("str");
	ASSERT_TRUE(p != hxnull);
	ASSERT_TRUE(::strcmp(p, "str") == 0);
	hxFree(p);
}

#if !(HX_MEMORY_MANAGER_DISABLE)

class hxMemoryManagerTest :
	public testing::Test
{
public:
	void TestMemoryAllocatorNormal(hxMemoryAllocator id) {
		uintptr_t startCount;
		uintptr_t startBytes;

		{
			hxMemoryAllocatorScope allocatorScope(id);

			startCount = allocatorScope.getTotalAllocationCount();
			startBytes = allocatorScope.getTotalBytesAllocated();

			void* ptr1 = hxMalloc(100);
			void* ptr2 = hxMalloc(200);
			::memset(ptr1, 0x33, 100);
			::memset(ptr2, 0x33, 200);

			{
				// Google Test spams new/delete with std::string operations:
				hxMemoryAllocatorScope spamGuard(hxMemoryAllocator_Heap);
				ASSERT_EQ(allocatorScope.getScopeAllocationCount(), 2u);
				ASSERT_EQ(allocatorScope.getPreviousAllocationCount(), startCount);
				ASSERT_EQ(allocatorScope.getTotalAllocationCount(), 2u + startCount);
				ASSERT_NEAR(allocatorScope.getScopeBytesAllocated(), 300u, 2u * HX_ALIGNMENT);
				ASSERT_NEAR(allocatorScope.getTotalBytesAllocated(), startBytes + 300u, 2u * HX_ALIGNMENT);
				ASSERT_EQ(allocatorScope.getPreviousBytesAllocated(), startBytes);
			}

			// Allow quiet deletion of a resource.
			g_hxSettings.deallocatePermanent = true;
			hxFree(ptr1);
			hxFree(ptr2);
			g_hxSettings.deallocatePermanent = false;

			// Special case for heaps that do not track free.
			if (allocatorScope.getScopeBytesAllocated() != 0) {
				// Google Test spams new/delete with std::string operations:
				hxMemoryAllocatorScope spamGuard(hxMemoryAllocator_Heap);

				// The debug heap requires HX_ALLOCATIONS_LOG_LEVEL enabled to track bytes allocated.
				ASSERT_NEAR(allocatorScope.getScopeBytesAllocated(), 300, 2 * HX_ALIGNMENT);
			}
			else {
				hxMemoryAllocatorScope spamGuard(hxMemoryAllocator_Heap);
				ASSERT_EQ(allocatorScope.getScopeBytesAllocated(), 0u);
				ASSERT_EQ(allocatorScope.getTotalBytesAllocated(), startBytes);
			}
		}

		// hxMemoryAllocator_Permanent does not free.
		if (id != hxMemoryAllocator_Permanent) {
			hxMemoryAllocatorScope allocatorScope(id);

			// Google Test spams new/delete with std::string operations:
			hxMemoryAllocatorScope spamGuard(hxMemoryAllocator_Heap);
			ASSERT_EQ(allocatorScope.getPreviousAllocationCount(), startCount);
			ASSERT_EQ(allocatorScope.getPreviousBytesAllocated(), startBytes);
		}
	}

	void TestMemoryAllocatorLeak(hxMemoryAllocator id) {
		(void)id;
#if (HX_RELEASE) < 1
		uintptr_t startCount = 0;
		uintptr_t startBytes = 0;
		void* ptr2 = hxnull;
		size_t assertsAllowed = g_hxSettings.assertsToBeSkipped;

		{
			hxMemoryAllocatorScope allocatorScope(id);

			ASSERT_EQ(0u, allocatorScope.getScopeAllocationCount());
			ASSERT_EQ(0u, allocatorScope.getScopeBytesAllocated());

			// Track the starting state to see how it is affected by a leak.
			startCount = allocatorScope.getPreviousAllocationCount();
			startBytes = allocatorScope.getPreviousBytesAllocated();

			void* ptr1 = hxMalloc(100);
			ptr2 = hxMalloc(200);
			::memset(ptr1, 0x33, 100);
			::memset(ptr2, 0x33, 200);

			hxFree(ptr1); // Only free the one.

			g_hxSettings.assertsToBeSkipped = 1;
		}
		ASSERT_EQ(g_hxSettings.assertsToBeSkipped, 0); // hxAssert was hit, leak in scope

		{
			hxMemoryAllocatorScope allocatorScope(id);

			// the allocator knows it has an outstanding allocation
			ASSERT_EQ(allocatorScope.getPreviousAllocationCount(), startCount + 1);

			// however the allocated memory was reset.
			ASSERT_EQ(allocatorScope.getPreviousBytesAllocated(), startBytes);

			g_hxSettings.assertsToBeSkipped = 1;
			hxFree(ptr2);
		}

		// hxAssert was hit, free after scope closed
		ASSERT_EQ(g_hxSettings.assertsToBeSkipped, 0);

		g_hxSettings.assertsToBeSkipped = assertsAllowed;
#endif
	}
};

TEST_F(hxMemoryManagerTest, Execute) {
	// The API should still work while stubbed out.
	for (size_t i = 0; i < hxMemoryAllocator_Current; ++i) {
		TestMemoryAllocatorNormal((hxMemoryAllocator)i);
	}

	// Leak checking requires the memory manager.
#if !(HX_MEMORY_MANAGER_DISABLE)
	hxLog("TEST_EXPECTING_ASSERTS:\n");
	// Only the TemporaryStack expects all allocations to be free()'d.
	TestMemoryAllocatorLeak(hxMemoryAllocator_TemporaryStack);
#endif
}

TEST_F(hxMemoryManagerTest, TempOverflow) {
	// there is no policy against using the debug heap in release
	void* p = hxMallocExt(HX_MEMORY_BUDGET_TEMPORARY_STACK + 1, hxMemoryAllocator_TemporaryStack, 0u);
	ASSERT_TRUE(p != hxnull);
	hxFree(p);

	hxMemoryAllocatorScope temp(hxMemoryAllocator_TemporaryStack);
	p = hxMalloc(HX_MEMORY_BUDGET_TEMPORARY_STACK + 1);
	ASSERT_TRUE(p != hxnull);
	hxFree(p);
}

#endif // !HX_MEMORY_MANAGER_DISABLE
