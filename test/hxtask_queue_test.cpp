// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include <hx/hxtask_queue.hpp>
#include <hx/hxtest.hpp>

HX_REGISTER_FILENAME_HASH

class hxtask_queue_test_f :
	public testing::Test
{
public:
	enum : uint8_t {
		max_pool = 8,
		max_tasks = 20
	};

	~hxtask_queue_test_f(void) {
	}
	class hxtask_test_t : public hxtask {
	public:
		hxtask_test_t() : m_exec_count(0), m_reenqueue_count(0) { }

		virtual void execute(hxtask_queue* q) override {
			++m_exec_count;
			if(m_reenqueue_count > 0) {
				--m_reenqueue_count;
				q->enqueue(this);
			}
		}

		size_t get_exec_count(void) { return m_exec_count; }
		void set_reenqueue_count(size_t n) { m_reenqueue_count = n; }

	private:
		size_t m_exec_count;
		size_t m_reenqueue_count;
	};
};

TEST_F(hxtask_queue_test_f, nop) {
	hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);

	for(size_t i = 0; i <= max_pool; ++i) {
		{
			hxtask_queue q(i);
		}
		{
			hxtask_queue q(i);
			q.wait_for_all();
		}
	}
	EXPECT_TRUE(true);
}

TEST_F(hxtask_queue_test_f, single) {
	hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);

	for(size_t i = 0; i <= max_pool; ++i) {
		hxtask_test_t task0;
		hxtask_test_t task1;
		{
			hxtask_queue q(i);
			q.enqueue(&task0);
			q.wait_for_all();
			q.enqueue(&task1);
			EXPECT_TRUE(task0.get_exec_count() == 1);
		}
		EXPECT_TRUE(task0.get_exec_count() == 1);
		EXPECT_TRUE(task1.get_exec_count() == 1);

		hxtask_test_t task2;
		{
			hxtask_queue q(i);
			q.enqueue(&task2);
		}
		EXPECT_TRUE(task2.get_exec_count() == 1);
	}
}

TEST_F(hxtask_queue_test_f, single_stepping) {
	hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);

	for(size_t i = 0; i <= max_pool; ++i) {
		for(size_t j = 1; j < max_tasks; ++j) {
			hxtask_test_t task0;
			{
				hxtask_queue q(i);
				for(size_t k = 1; k <= j; ++k) {
					q.enqueue(&task0);
					q.wait_for_all();
				}
				EXPECT_TRUE(task0.get_exec_count() == j);
			}
			EXPECT_TRUE(task0.get_exec_count() == j);
		}
	}
}

TEST_F(hxtask_queue_test_f, multiple) {
	hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);

	for(size_t i = 0; i <= max_pool; ++i) {
		for(size_t j = 1; j < max_tasks; ++j) {

			hxtask_test_t tasks0[max_tasks];
			hxtask_test_t tasks1[max_tasks];
			{
				hxtask_queue q(i);
				for(size_t k = 0; k <= j; ++k) {
					q.enqueue(&tasks0[k]);
				}
				q.wait_for_all();
				for(size_t k = 0; k <= j; ++k) {
					q.enqueue(&tasks1[k]);
					EXPECT_TRUE(tasks0[k].get_exec_count() == 1);
				}
			}
			for(size_t k = 0; k <= j; ++k) {
				EXPECT_TRUE(tasks0[k].get_exec_count() == 1);
				EXPECT_TRUE(tasks1[k].get_exec_count() == 1);
			}

			hxtask_test_t tasks2[max_tasks];
			{
				hxtask_queue q(i);
				for(size_t k = 0; k <= j; ++k) {
					q.enqueue(&tasks2[k]);
				}
			}
			for(size_t k = 0; k <= j; ++k) {
				EXPECT_TRUE(tasks2[k].get_exec_count() == 1);
			}
		}
	}
}

TEST_F(hxtask_queue_test_f, multiple_stepping) {
	hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);

	for(size_t i = 0; i <= max_pool; ++i) {
		for(size_t j = 1; j < max_tasks; ++j) {

			hxtask_test_t tasks0[max_tasks];
			{
				hxtask_queue q(i);
				for(size_t k = 1; k <= j; ++k) {
					for(size_t l = 0; l <= j; ++l) {
						q.enqueue(&tasks0[l]);
					}
					q.wait_for_all();
				}
			}
			for(size_t l = 0; l <= j; ++l) {
				EXPECT_TRUE(tasks0[l].get_exec_count() == j);
			}
		}
	}
}

TEST_F(hxtask_queue_test_f, multiple_reenqueuing) {
	hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);

	for(size_t i = 0; i <= max_pool; ++i) {
		for(size_t j = 1; j < max_tasks; ++j) {

			hxtask_test_t tasks0[max_tasks];
			hxtask_test_t tasks1[max_tasks];
			{
				hxtask_queue q(i);
				for(size_t k = 0; k <= j; ++k) {
					tasks0[k].set_reenqueue_count(k);
					q.enqueue(&tasks0[k]);
				}
				q.wait_for_all();
				for(size_t k = 0; k <= j; ++k) {
					tasks1[k].set_reenqueue_count(k);
					q.enqueue(&tasks1[k]);
				}
			}
			for(size_t k = 0; k <= j; ++k) {
				EXPECT_TRUE(tasks0[k].get_exec_count() == (k + 1));
				EXPECT_TRUE(tasks1[k].get_exec_count() == (k + 1));
			}

			// Tests reenqueuing in the destructor.
			hxtask_test_t tasks2[max_tasks];
			{
				hxtask_queue q(i);
				for(size_t k = 0; k <= j; ++k) {
					tasks2[k].set_reenqueue_count(k);
					q.enqueue(&tasks2[k]);
				}
			}
			for(size_t k = 0; k <= j; ++k) {
				EXPECT_TRUE(tasks2[k].get_exec_count() == (k + 1));
			}
		}
	}
}
