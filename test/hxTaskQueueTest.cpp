// Copyright 2017-2025 Adrian Johnston

#include <hx/hxTaskQueue.hpp>
#include <hx/hxTest.hpp>

HX_REGISTER_FILENAME_HASH

class hxTaskQueueTest :
	public testing::Test
{
public:
	enum {
		MAX_POOL = 8,
		MAX_TASKS = 20
	};

    ~hxTaskQueueTest() {
    }
    struct TaskTest : public hxTask {
        TaskTest() : m_execCount_(0), m_reenqueueCount_(0) { }

        virtual void execute(hxTaskQueue* q) HX_OVERRIDE {
            ++m_execCount_;
            if (m_reenqueueCount_ > 0) {
                --m_reenqueueCount_;
                q->enqueue(this);
            }
        }

        size_t m_execCount_;
        size_t m_reenqueueCount_;
    };
};

TEST_F(hxTaskQueueTest, Nop) {
	for (size_t i = 0; i <= MAX_POOL; ++i) {
		{
			hxTaskQueue q(i);
		}
		{
			hxTaskQueue q(i);
			q.waitForAll();
		}
	}
	ASSERT_TRUE(true);
}

TEST_F(hxTaskQueueTest, Single) {
	for (size_t i = 0; i <= MAX_POOL; ++i) {
		TaskTest task0;
		TaskTest task1;
		{
			hxTaskQueue q(i);
			q.enqueue(&task0);
			q.waitForAll();
			q.enqueue(&task1);
			ASSERT_TRUE(task0.m_execCount_ == 1);
		}
		ASSERT_TRUE(task0.m_execCount_ == 1);
		ASSERT_TRUE(task1.m_execCount_ == 1);

		TaskTest task2;
		{
			hxTaskQueue q(i);
			q.enqueue(&task2);
		}
		ASSERT_TRUE(task2.m_execCount_ == 1);
	}
}

TEST_F(hxTaskQueueTest, SingleStepping) {
	for (size_t i = 0; i <= MAX_POOL; ++i) {
		for (size_t j = 1; j < MAX_TASKS; ++j) {
			TaskTest task0;
			{
				hxTaskQueue q(i);
				for (size_t k = 1; k <= j; ++k) {
					q.enqueue(&task0);
					q.waitForAll();
				}
				ASSERT_TRUE(task0.m_execCount_ == j);
			}
			ASSERT_TRUE(task0.m_execCount_ == j);
		}
	}
}

TEST_F(hxTaskQueueTest, Multiple) {
	for (size_t i = 0; i <= MAX_POOL; ++i) {
		for (size_t j = 1; j < MAX_TASKS; ++j) {

			TaskTest tasks0[MAX_TASKS];
			TaskTest tasks1[MAX_TASKS];
			{
				hxTaskQueue q(i);
				for (size_t k = 0; k <= j; ++k) {
					q.enqueue(&tasks0[k]);
				}
				q.waitForAll();
				for (size_t k = 0; k <= j; ++k) {
					q.enqueue(&tasks1[k]);
					ASSERT_TRUE(tasks0[k].m_execCount_ == 1);
				}
			}
			for (size_t k = 0; k <= j; ++k) {
				ASSERT_TRUE(tasks0[k].m_execCount_ == 1);
				ASSERT_TRUE(tasks1[k].m_execCount_ == 1);
			}

			TaskTest tasks2[MAX_TASKS];
			{
				hxTaskQueue q(i);
				for (size_t k = 0; k <= j; ++k) {
					q.enqueue(&tasks2[k]);
				}
			}
			for (size_t k = 0; k <= j; ++k) {
				ASSERT_TRUE(tasks2[k].m_execCount_ == 1);
			}
		}
	}
}

TEST_F(hxTaskQueueTest, MultipleStepping) {
	for (size_t i = 0; i <= MAX_POOL; ++i) {
		for (size_t j = 1; j < MAX_TASKS; ++j) {

			TaskTest tasks0[MAX_TASKS];
			{
				hxTaskQueue q(i);
				for (size_t k = 1; k <= j; ++k) {
					for (size_t l = 0; l <= j; ++l) {
						q.enqueue(&tasks0[l]);
					}
					q.waitForAll();
				}
			}
			for (size_t l = 0; l <= j; ++l) {
				ASSERT_TRUE(tasks0[l].m_execCount_ == j);
			}
		}
	}
}

TEST_F(hxTaskQueueTest, MultipleReenqueuing) {
	for (size_t i = 0; i <= MAX_POOL; ++i) {
		for (size_t j = 1; j < MAX_TASKS; ++j) {

			TaskTest tasks0[MAX_TASKS];
			TaskTest tasks1[MAX_TASKS];
			{
				hxTaskQueue q(i);
				for (size_t k = 0; k <= j; ++k) {
					tasks0[k].m_reenqueueCount_ = k;
					q.enqueue(&tasks0[k]);
				}
				q.waitForAll();
				for (size_t k = 0; k <= j; ++k) {
					tasks1[k].m_reenqueueCount_ = k;
					q.enqueue(&tasks1[k]);
				}
			}
			for (size_t k = 0; k <= j; ++k) {
				ASSERT_TRUE(tasks0[k].m_execCount_ == (k + 1));
				ASSERT_TRUE(tasks1[k].m_execCount_ == (k + 1));
			}

			// Tests reenqueuing in destructor
			TaskTest tasks2[MAX_TASKS];
			{
				hxTaskQueue q(i);
				for (size_t k = 0; k <= j; ++k) {
					tasks2[k].m_reenqueueCount_ = k;
					q.enqueue(&tasks2[k]);
				}
			}
			for (size_t k = 0; k <= j; ++k) {
				ASSERT_TRUE(tasks2[k].m_execCount_ == (k + 1));
			}
		}
	}
}
