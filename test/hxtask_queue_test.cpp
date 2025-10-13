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
			hxtask_queue q(1, i);
		}
		{
			hxtask_queue q(1, i);
			q.wait_for_all();
		}
	}
	EXPECT_TRUE(true);
}

TEST_F(hxtask_queue_test_f, multiple) {
	hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);

	for(size_t i = 0; i <= max_pool; ++i) {
		for(size_t j = 1; j < max_tasks; ++j) {

			hxtask_test_t tasks0[max_tasks];
			hxtask_test_t tasks1[max_tasks];
			{
				hxtask_queue q(max_tasks, i);
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
				hxtask_queue q(max_tasks, i);
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

TEST_F(hxtask_queue_test_f, multiple_reenqueuing) {
	hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);

	for(size_t i = 0; i <= max_pool; ++i) {
		for(size_t j = 1; j < max_tasks; ++j) {

			hxtask_test_t tasks0[max_tasks];
			hxtask_test_t tasks1[max_tasks];
			{
				hxtask_queue q(max_tasks, i);
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
				hxtask_queue q(max_tasks, i);
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

TEST(hxtask_queue_test, priority_ordering_single_threaded) {
	hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);

	class hxtask_queue_test_task_t : public hxtask {
	public:
		void configure(int* o, size_t* i, int p) {
			execution_order = o;
			write_index = i;
			priority_value = p;
		}

		virtual void execute(hxtask_queue*) override {
			hxassertmsg(execution_order, "priority_task_unconfigured");
			hxassertmsg(write_index, "priority_task_unconfigured");
			size_t slot = (*write_index)++;
			execution_order[slot] = priority_value;
		}

	private:
		int* execution_order = hxnull;
		size_t* write_index = hxnull;
		int priority_value = 0;
	};

	constexpr size_t task_count = 5;
	int execution_order[task_count] = { 0, 0, 0, 0, 0 };
	size_t write_index = 0;
	hxtask_queue_test_task_t tasks[task_count];

	const int priorities[task_count] = { 1, 3, -5, 2, 10 };

	hxtask_queue q(task_count, 0);
	for(size_t i = 0; i < task_count; ++i) {
		tasks[i].configure(execution_order, &write_index, priorities[i]);
		q.enqueue(&tasks[i], priorities[i]);
	}
	q.wait_for_all();

	EXPECT_TRUE(write_index == task_count);

	const int expected[task_count] = { 10, 3, 2, 1, -5 };
	for(size_t i = 0; i < task_count; ++i) {
		EXPECT_TRUE(execution_order[i] == expected[i]);
	}
}

TEST(hxtask_queue_test, predicates_cover_all_any_erase) {
	hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);

	class hxtask_queue_predicate_task_t : public hxtask {
	public:
		void configure(bool* executed_flag_) {
			executed_flag = executed_flag_;
		}

		virtual void execute(hxtask_queue*) override {
			*executed_flag = true;
		}

	private:
		bool* executed_flag = hxnull;
	};

	bool executed_flags[3] = { false, false, false };
	hxtask_queue_predicate_task_t tasks[3];
	for(size_t i = 0; i < 3; ++i) {
		tasks[i].configure(&executed_flags[i]);
	}

	hxtask_queue q(3, 0);
	q.enqueue(&tasks[0], 5);
	q.enqueue(&tasks[1], 10);
	q.enqueue(&tasks[2], 1);

	bool all_priority_non_negative = q.all_of([](const hxtask_queue::task_record_t& record_) {
		return record_.priority >= 0;
	});
	EXPECT_TRUE(all_priority_non_negative);

	bool any_high_priority = q.any_of([](const hxtask_queue::task_record_t& record_) {
		return record_.priority > 8;
	});
	EXPECT_TRUE(any_high_priority);

	size_t removed_low_priority = q.erase_if([](const hxtask_queue::task_record_t& record_) {
		return record_.priority < 4;
	});
	EXPECT_TRUE(removed_low_priority == 1);

	bool any_remaining_low_priority = q.any_of([](const hxtask_queue::task_record_t& record_) {
		return record_.priority < 4;
	});
	EXPECT_TRUE(!any_remaining_low_priority);

	q.wait_for_all();

	EXPECT_TRUE(executed_flags[0]);
	EXPECT_TRUE(executed_flags[1]);
	EXPECT_TRUE(!executed_flags[2]);
}
