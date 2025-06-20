// Copyright 2017-2025 Adrian Johnston

#include <hx/hxtask_queue.hpp>
#include <hx/hxtest.hpp>

HX_REGISTER_FILENAME_HASH

class hxtask_queue_test :
	public testing::Test
{
public:
	enum {
		MAX_POOL = 8,
		MAX_TASKS = 20
	};

    ~hxtask_queue_test() {
    }
    struct Task_test : public hxtask {
        Task_test() : m_exec_count_(0), m_reenqueue_count_(0) { }

        virtual void execute(hxtask_queue* q) HX_OVERRIDE {
            ++m_exec_count_;
            if (m_reenqueue_count_ > 0) {
                --m_reenqueue_count_;
                q->enqueue(this);
            }
        }

        size_t m_exec_count_;
        size_t m_reenqueue_count_;
    };
};

TEST_F(hxtask_queue_test, Nop) {
	for (size_t i = 0; i <= MAX_POOL; ++i) {
		{
			hxtask_queue q(i);
		}
		{
			hxtask_queue q(i);
			q.wait_for_all();
		}
	}
	ASSERT_TRUE(true);
}

TEST_F(hxtask_queue_test, Single) {
	for (size_t i = 0; i <= MAX_POOL; ++i) {
		Task_test task0;
		Task_test task1;
		{
			hxtask_queue q(i);
			q.enqueue(&task0);
			q.wait_for_all();
			q.enqueue(&task1);
			ASSERT_TRUE(task0.m_exec_count_ == 1);
		}
		ASSERT_TRUE(task0.m_exec_count_ == 1);
		ASSERT_TRUE(task1.m_exec_count_ == 1);

		Task_test task2;
		{
			hxtask_queue q(i);
			q.enqueue(&task2);
		}
		ASSERT_TRUE(task2.m_exec_count_ == 1);
	}
}

TEST_F(hxtask_queue_test, Single_stepping) {
	for (size_t i = 0; i <= MAX_POOL; ++i) {
		for (size_t j = 1; j < MAX_TASKS; ++j) {
			Task_test task0;
			{
				hxtask_queue q(i);
				for (size_t k = 1; k <= j; ++k) {
					q.enqueue(&task0);
					q.wait_for_all();
				}
				ASSERT_TRUE(task0.m_exec_count_ == j);
			}
			ASSERT_TRUE(task0.m_exec_count_ == j);
		}
	}
}

TEST_F(hxtask_queue_test, Multiple) {
	for (size_t i = 0; i <= MAX_POOL; ++i) {
		for (size_t j = 1; j < MAX_TASKS; ++j) {

			Task_test tasks0[MAX_TASKS];
			Task_test tasks1[MAX_TASKS];
			{
				hxtask_queue q(i);
				for (size_t k = 0; k <= j; ++k) {
					q.enqueue(&tasks0[k]);
				}
				q.wait_for_all();
				for (size_t k = 0; k <= j; ++k) {
					q.enqueue(&tasks1[k]);
					ASSERT_TRUE(tasks0[k].m_exec_count_ == 1);
				}
			}
			for (size_t k = 0; k <= j; ++k) {
				ASSERT_TRUE(tasks0[k].m_exec_count_ == 1);
				ASSERT_TRUE(tasks1[k].m_exec_count_ == 1);
			}

			Task_test tasks2[MAX_TASKS];
			{
				hxtask_queue q(i);
				for (size_t k = 0; k <= j; ++k) {
					q.enqueue(&tasks2[k]);
				}
			}
			for (size_t k = 0; k <= j; ++k) {
				ASSERT_TRUE(tasks2[k].m_exec_count_ == 1);
			}
		}
	}
}

TEST_F(hxtask_queue_test, Multiple_stepping) {
	for (size_t i = 0; i <= MAX_POOL; ++i) {
		for (size_t j = 1; j < MAX_TASKS; ++j) {

			Task_test tasks0[MAX_TASKS];
			{
				hxtask_queue q(i);
				for (size_t k = 1; k <= j; ++k) {
					for (size_t l = 0; l <= j; ++l) {
						q.enqueue(&tasks0[l]);
					}
					q.wait_for_all();
				}
			}
			for (size_t l = 0; l <= j; ++l) {
				ASSERT_TRUE(tasks0[l].m_exec_count_ == j);
			}
		}
	}
}

TEST_F(hxtask_queue_test, Multiple_reenqueuing) {
	for (size_t i = 0; i <= MAX_POOL; ++i) {
		for (size_t j = 1; j < MAX_TASKS; ++j) {

			Task_test tasks0[MAX_TASKS];
			Task_test tasks1[MAX_TASKS];
			{
				hxtask_queue q(i);
				for (size_t k = 0; k <= j; ++k) {
					tasks0[k].m_reenqueue_count_ = k;
					q.enqueue(&tasks0[k]);
				}
				q.wait_for_all();
				for (size_t k = 0; k <= j; ++k) {
					tasks1[k].m_reenqueue_count_ = k;
					q.enqueue(&tasks1[k]);
				}
			}
			for (size_t k = 0; k <= j; ++k) {
				ASSERT_TRUE(tasks0[k].m_exec_count_ == (k + 1));
				ASSERT_TRUE(tasks1[k].m_exec_count_ == (k + 1));
			}

			// Tests reenqueuing in destructor
			Task_test tasks2[MAX_TASKS];
			{
				hxtask_queue q(i);
				for (size_t k = 0; k <= j; ++k) {
					tasks2[k].m_reenqueue_count_ = k;
					q.enqueue(&tasks2[k]);
				}
			}
			for (size_t k = 0; k <= j; ++k) {
				ASSERT_TRUE(tasks2[k].m_exec_count_ == (k + 1));
			}
		}
	}
}
