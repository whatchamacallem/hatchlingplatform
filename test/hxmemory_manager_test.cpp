// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>
#include <hx/hxtest.hpp>

HX_REGISTER_FILENAME_HASH

// Using ASSERT_* instead of EXPECT_* in this file to add coverage for those
// macros. Memory corruption sounds fatal so sure why not. Some of these tests
// are designed to fail and use EXPECT_ for those specific tests.

TEST(hxmemory_manager_test_f, Bytes) {
	for(size_t i=0u; i<10u; ++i) {
		void* p = hxmalloc(i);
		ASSERT_TRUE(p != hxnull);
		::memset(p, 0x66, i);
		hxfree(p);
	}
}

TEST(hxmemory_manager_test_f, String_duplicate_null) {
	// duplicating a null string is null.
	void* p = hxstring_duplicate(hxnull);
	ASSERT_TRUE(p == hxnull);
	hxfree(hxnull);
}

TEST(hxmemory_manager_test_f, String_duplicate) {
	char* p = hxstring_duplicate("str");
	ASSERT_TRUE(p != hxnull);
	ASSERT_TRUE(::strcmp(p, "str") == 0);
	hxfree(p);
}

#if !(HX_MEMORY_MANAGER_DISABLE)

class hxmemory_manager_test :
	public testing::Test
{
public:
	void test_memory_allocator_normal(hxmemory_allocator id) {
		uintptr_t start_count;
		uintptr_t start_bytes;

		{
			hxmemory_allocator_scope allocator_scope(id);

			start_count = allocator_scope.get_total_allocation_count();
			start_bytes = allocator_scope.get_total_bytes_allocated();

			void* ptr1 = hxmalloc(100);
			void* ptr2 = hxmalloc(200);
			::memset(ptr1, 0x33, 100);
			::memset(ptr2, 0x33, 200);

			{
				// Google Test spams new/delete with std::string operations:
				hxmemory_allocator_scope spam_guard(hxmemory_allocator_heap);
				ASSERT_EQ(allocator_scope.get_scope_allocation_count(), 2u);
				ASSERT_EQ(allocator_scope.get_previous_allocation_count(), start_count);
				ASSERT_EQ(allocator_scope.get_total_allocation_count(), 2u + start_count);
				ASSERT_NEAR(allocator_scope.get_scope_bytes_allocated(), 300u, 2u * HX_ALIGNMENT);
				ASSERT_NEAR(allocator_scope.get_total_bytes_allocated(), start_bytes + 300u, 2u * HX_ALIGNMENT);
				ASSERT_EQ(allocator_scope.get_previous_bytes_allocated(), start_bytes);
			}

			// Allow quiet deletion of a resource.
			g_hxsettings.deallocate_permanent = true;
			hxfree(ptr1);
			hxfree(ptr2);
			g_hxsettings.deallocate_permanent = false;

			// Special case for heaps that do not track free.
			if (allocator_scope.get_scope_bytes_allocated() != 0) {
				// Google Test spams new/delete with std::string operations:
				hxmemory_allocator_scope spam_guard(hxmemory_allocator_heap);

				// The debug heap requires HX_ALLOCATIONS_LOG_LEVEL enabled to track bytes allocated.
				ASSERT_NEAR(allocator_scope.get_scope_bytes_allocated(), 300, 2 * HX_ALIGNMENT);
			}
			else {
				hxmemory_allocator_scope spam_guard(hxmemory_allocator_heap);
				ASSERT_EQ(allocator_scope.get_scope_bytes_allocated(), 0u);
				ASSERT_EQ(allocator_scope.get_total_bytes_allocated(), start_bytes);
			}
		}

		// hxmemory_allocator_permanent does not free.
		if (id != hxmemory_allocator_permanent) {
			hxmemory_allocator_scope allocator_scope(id);

			// Google Test spams new/delete with std::string operations:
			hxmemory_allocator_scope spam_guard(hxmemory_allocator_heap);
			ASSERT_EQ(allocator_scope.get_previous_allocation_count(), start_count);
			ASSERT_EQ(allocator_scope.get_previous_bytes_allocated(), start_bytes);
		}
	}

	void test_memory_allocator_leak(hxmemory_allocator id) {
		(void)id;
#if (HX_RELEASE) < 1
		uintptr_t start_count = 0;
		uintptr_t start_bytes = 0;
		void* ptr2 = hxnull;
		size_t asserts_allowed = g_hxsettings.asserts_to_be_skipped;

		{
			hxmemory_allocator_scope allocator_scope(id);

			ASSERT_EQ(0u, allocator_scope.get_scope_allocation_count());
			ASSERT_EQ(0u, allocator_scope.get_scope_bytes_allocated());

			// Track the starting state to see how it is affected by a leak.
			start_count = allocator_scope.get_previous_allocation_count();
			start_bytes = allocator_scope.get_previous_bytes_allocated();

			void* ptr1 = hxmalloc(100);
			ptr2 = hxmalloc(200);
			::memset(ptr1, 0x33, 100);
			::memset(ptr2, 0x33, 200);

			hxfree(ptr1); // Only free the one.

			g_hxsettings.asserts_to_be_skipped = 1;
		}
		ASSERT_EQ(g_hxsettings.asserts_to_be_skipped, 0); // hxassert was hit, leak in scope

		{
			hxmemory_allocator_scope allocator_scope(id);

			// the allocator knows it has an outstanding allocation
			ASSERT_EQ(allocator_scope.get_previous_allocation_count(), start_count + 1);

			// however the allocated memory was reset.
			ASSERT_EQ(allocator_scope.get_previous_bytes_allocated(), start_bytes);

			g_hxsettings.asserts_to_be_skipped = 1;
			hxfree(ptr2);
		}

		// hxassert was hit, free after scope closed
		ASSERT_EQ(g_hxsettings.asserts_to_be_skipped, 0);

		g_hxsettings.asserts_to_be_skipped = asserts_allowed;
#endif
	}
};

TEST_F(hxmemory_manager_test, Execute) {
	// The API should still work while stubbed out.
	for (size_t i = 0; i < hxmemory_allocator_current; ++i) {
		test_memory_allocator_normal((hxmemory_allocator)i);
	}

	// Leak checking requires the memory manager.
#if !(HX_MEMORY_MANAGER_DISABLE)
	hxlog("test_expecting_asserts:\n");
	// Only the Temporary_stack expects all allocations to be free()'d.
	test_memory_allocator_leak(hxmemory_allocator_temporary_stack);
#endif
}

TEST_F(hxmemory_manager_test, Temp_overflow) {
	// there is no policy against using the debug heap in release
	void* p = hxmalloc_ext(HX_MEMORY_BUDGET_TEMPORARY_STACK + 1, hxmemory_allocator_temporary_stack, 0u);
	ASSERT_TRUE(p != hxnull);
	hxfree(p);

	hxmemory_allocator_scope temp(hxmemory_allocator_temporary_stack);
	p = hxmalloc(HX_MEMORY_BUDGET_TEMPORARY_STACK + 1);
	ASSERT_TRUE(p != hxnull);
	hxfree(p);
}

#endif // !HX_MEMORY_MANAGER_DISABLE
