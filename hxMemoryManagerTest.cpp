// Copyright 2017 Adrian Johnston
// Copyright 2017 Leap Motion

#include "hatchling.h"
#include "hxTest.h"
#include "hxMemoryManager.h"

HX_REGISTER_FILENAME_HASH

// ----------------------------------------------------------------------------------

class hxMemoryManagerTest :
	public testing::test
{
public:
	void TestMemoryAllocatorNormal(hxMemoryManagerId id) {
		uintptr_t startCount;
		uintptr_t startBytes;

		{
			hxLog("hxTestMemoryAllocatorNormal %d...\n", (int)id);
			hxMemoryManagerScope resourceAllocator(id);

			startCount = resourceAllocator.getTotalAllocationCount();
			startBytes = resourceAllocator.getTotalBytesAllocated();

			void* ptr1 = hxMalloc(100);
			void* ptr2 = hxMalloc(200);
			::memset(ptr1, 0x33, 100);
			::memset(ptr2, 0x33, 200);

			{
				// GoogleTest spams new/delete with std::string operations:
				hxMemoryManagerScope spamGuard(hxMemoryManagerId_Heap);
				ASSERT_EQ(resourceAllocator.getScopeAllocationCount(), 2u);
				ASSERT_EQ(resourceAllocator.getPreviousAllocationCount(), startCount);
				ASSERT_EQ(resourceAllocator.getTotalAllocationCount(), 2u + startCount);
				ASSERT_NEAR(resourceAllocator.getScopeBytesAllocated(), 300u, 2u * HX_ALIGNMENT_MASK);
				ASSERT_NEAR(resourceAllocator.getTotalBytesAllocated(), startBytes + 300u, 2u * HX_ALIGNMENT_MASK);
				ASSERT_EQ(resourceAllocator.getPreviousBytesAllocated(), startBytes);
			}

			// Allow quiet deletion of a resource.
			g_hxSettings.isShuttingDown = true;
			hxFree(ptr1);
			hxFree(ptr2);
			g_hxSettings.isShuttingDown = false;

			// Special case for heaps that do not track free.
			if (resourceAllocator.getScopeBytesAllocated() != 0) {
				// GoogleTest spams new/delete with std::string operations:
				hxMemoryManagerScope spamGuard(hxMemoryManagerId_Heap);

				// The debug heap requires HX_ALLOCATIONS_LOG_LEVEL enabled to track bytes allocated.
				ASSERT_NEAR(resourceAllocator.getScopeBytesAllocated(), 300, 2 * HX_ALIGNMENT_MASK);
			}
			else {
				hxMemoryManagerScope spamGuard(hxMemoryManagerId_Heap);
				ASSERT_EQ(resourceAllocator.getScopeBytesAllocated(), 0u);
				ASSERT_EQ(resourceAllocator.getTotalBytesAllocated(), startBytes);
			}
		}

		// hxMemoryManagerId_Permanent does not free.
		if (id != hxMemoryManagerId_Permanent) {
			hxMemoryManagerScope resourceAllocator(id);

			// GoogleTest spams new/delete with std::string operations:
			hxMemoryManagerScope spamGuard(hxMemoryManagerId_Heap);
			ASSERT_EQ(resourceAllocator.getPreviousAllocationCount(), startCount);
			ASSERT_EQ(resourceAllocator.getPreviousBytesAllocated(), startBytes);
		}
	}

	void TestMemoryAllocatorLeak(hxMemoryManagerId id) {
#if (HX_RELEASE) < 1
		uintptr_t startCount = 0;
		uintptr_t startBytes = 0;
		void* ptr2 = hxnull;
		int32_t assertsAllowed = g_hxSettings.assertsToBeSkipped;

		{
			hxMemoryManagerScope resourceAllocator(id);

			startCount = resourceAllocator.getScopeAllocationCount();
			startBytes = resourceAllocator.getScopeBytesAllocated();

			void* ptr1 = hxMalloc(100);
			ptr2 = hxMalloc(200);
			::memset(ptr1, 0x33, 100);
			::memset(ptr2, 0x33, 200);

			hxFree(ptr1); // Only free the one.

			g_hxSettings.assertsToBeSkipped = 1;
		}
		ASSERT_EQ(g_hxSettings.assertsToBeSkipped, 0); // hxAssert was hit, leak in scope

		hxMemoryManagerScope resourceAllocator(id);
		{
			// GoogleTest spams new/delete with std::string operations:
			hxMemoryManagerScope spamGuard(hxMemoryManagerId_Heap);
			ASSERT_EQ(resourceAllocator.getPreviousAllocationCount(), startCount);
			ASSERT_EQ(resourceAllocator.getPreviousBytesAllocated(), startBytes);
		}

		g_hxSettings.assertsToBeSkipped = 1;
		hxFree(ptr2);
		ASSERT_EQ(g_hxSettings.assertsToBeSkipped, 0); // hxAssert was hit, free after scope closed
		g_hxSettings.assertsToBeSkipped = assertsAllowed;
#endif
	}
};

// ----------------------------------------------------------------------------

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
