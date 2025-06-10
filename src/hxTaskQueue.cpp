// Copyright 2017-2025 Adrian Johnston

#include <hx/hxTaskQueue.hpp>
#include <hx/hxProfiler.hpp>

HX_REGISTER_FILENAME_HASH

// ----------------------------------------------------------------------------
// hxTaskQueue

hxTaskQueue::hxTaskQueue(int32_t threadPoolSize_)
    : m_nextTask_(hxnull)
    , m_runningQueueCheck_(RunningQueueCheck_)

{
    (void)threadPoolSize_;
#if HX_USE_CPP_THREADS
    m_threadPoolSize_ = (threadPoolSize_ >= 0) ? threadPoolSize_
        : ((int32_t)std::thread::hardware_concurrency() - 1);
    if (m_threadPoolSize_ > 0) {
        m_threads_ = (std::thread*)hxMalloc(m_threadPoolSize_ * sizeof(std::thread));
        for (int32_t i_ = m_threadPoolSize_; i_--;) {
            ::new (m_threads_ + i_) std::thread(executorThread_, this, ExecutorMode_::Pool_);
        }
    }
#endif
}

hxTaskQueue::~hxTaskQueue() {
#if HX_USE_CPP_THREADS
    if (m_threadPoolSize_ > 0) {
        // Contribute current thread, request waiting until completion and signal stopping.
        executorThread_(this, ExecutorMode_::Stopping_);
        hxAssertRelease(m_runningQueueCheck_ == 0u, "Q");

        for (int32_t i_ = m_threadPoolSize_; i_--;) {
            m_threads_[i_].join();
            m_threads_[i_].~thread();
        }
        hxFree(m_threads_);
        m_threads_ = hxnull;
    }
    else
#endif
    {
        waitForAll();
        m_runningQueueCheck_ = 0u;
    }
}

void hxTaskQueue::enqueue(hxTask* task_) {
    task_->setTaskQueue(this);

#if HX_USE_CPP_THREADS
    if (m_threadPoolSize_ > 0) {
        std::unique_lock<std::mutex> lock_(m_mutex_);
        hxAssertRelease(m_runningQueueCheck_ == RunningQueueCheck_, "enqueue to stopped queue");
        task_->setNextTask(m_nextTask_);
        m_nextTask_ = task_;
        m_condVarTasks_.notify_one();
    }
    else
#endif
    {
        task_->setNextTask(m_nextTask_);
        m_nextTask_ = task_;
    }
}

void hxTaskQueue::waitForAll() {
#if HX_USE_CPP_THREADS
    if (m_threadPoolSize_ > 0) {
        // Contribute current thread and request waiting until completion.
        executorThread_(this, ExecutorMode_::Waiting_);
    }
    else
#endif
    {
        while (m_nextTask_) {
            hxTask* task_ = m_nextTask_;
            m_nextTask_ = task_->getNextTask();
            task_->setNextTask(hxnull);
            task_->setTaskQueue(hxnull);

            // Last time this object is touched. It may delete or re-enqueue itself, we
            // don't care.
            hxProfileScope(task_->getLabel());
            task_->execute(this);
        }
    }
}

#if HX_USE_CPP_THREADS
void hxTaskQueue::executorThread_(hxTaskQueue* q_, ExecutorMode_ mode_) {
    hxTask* task_ = hxnull;
    for (;;) {
        {
            std::unique_lock<std::mutex> lk_(q_->m_mutex_);

            if (task_) {
                // Waited to reacquire critical section to decrement counter for previous task.
                task_ = hxnull;
                hxAssert(q_->m_executingCount_ > 0);
                if (--q_->m_executingCount_ == 0 && !q_->m_nextTask_) {
                    q_->m_condVarWaiting_.notify_all();
                }
            }

            // Either acquire a next task or meet stopping criteria.
            if (mode_ == ExecutorMode_::Pool_) {
                q_->m_condVarTasks_.wait(lk_, [q_] {
                    return q_->m_nextTask_ || q_->m_runningQueueCheck_ != RunningQueueCheck_;
                });
            }

            if (q_->m_nextTask_) {
                hxAssertRelease(q_->m_runningQueueCheck_ == RunningQueueCheck_, "Q");
                task_ = q_->m_nextTask_;
                q_->m_nextTask_ = task_->getNextTask();
                ++q_->m_executingCount_;
            }
            else {
                if (mode_ != ExecutorMode_::Pool_) {
                    q_->m_condVarWaiting_.wait(lk_, [q_] {
                        return q_->m_executingCount_ == 0 && !q_->m_nextTask_;
                    });

                    if (mode_ == ExecutorMode_::Stopping_) {
                        hxAssertRelease(q_->m_runningQueueCheck_ == RunningQueueCheck_, "Q");
                        q_->m_runningQueueCheck_ = 0u;
                        q_->m_condVarTasks_.notify_all();
                    }
                }

				return;
			}
		}

        task_->setNextTask(hxnull);
        task_->setTaskQueue(hxnull);
        hxProfileScope(task_->getLabel());
        // Last time this object is touched. It may delete or re-enqueue itself.
        task_->execute(q_);
    }
}
#endif
