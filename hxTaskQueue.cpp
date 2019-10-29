// Copyright 2017 Adrian Johnston

#include "hxTaskQueue.h"
#include "hxProfiler.h"

// ----------------------------------------------------------------------------

hxTaskQueue::hxTaskQueue(int32_t threadPoolSize)
	: m_nextWaitingTask(null)
	, m_runningQueueCheck(c_runningQueueCheck)

{
#if HX_HAS_CPP11_THREADS
	m_threadPoolSize = (threadPoolSize >= 0) ? threadPoolSize : ((int32_t)std::thread::hardware_concurrency() - 1);
	if (m_threadPoolSize > 0) {
		m_threads = (std::thread*)hxMalloc(m_threadPoolSize * sizeof(std::thread));
		for (int32_t i = m_threadPoolSize; i--;) {
			::new (m_threads + i) std::thread(executorThread, this, ExecutorMode::Pool);
		}
	}
#endif
}

hxTaskQueue::~hxTaskQueue() {
#if HX_HAS_CPP11_THREADS
	if (m_threadPoolSize > 0) {
		// Contribute current thread, request waiting until completion and signal stopping.
		executorThread(this, ExecutorMode::Stopping);
		hxAssert(m_runningQueueCheck == 0u);

		for (int32_t i = m_threadPoolSize; i--;) {
			m_threads[i].join();
			m_threads[i].~thread();
		}
		hxFree(m_threads);
		m_threads = null;
	}
	else
#endif
	{
		waitForAll();
		m_runningQueueCheck = 0u;
	}
}

void hxTaskQueue::enqueue(Task* task) {
	hxAssert(!task->m_queue && !task->m_nextWaitingTask);
	task->m_queue = this;

#if HX_HAS_CPP11_THREADS
	if (m_threadPoolSize > 0) {
		std::unique_lock<std::mutex> lk(m_mutex);
		hxAssertRelease(m_runningQueueCheck == c_runningQueueCheck, "enqueue to stopped queue");
		task->m_nextWaitingTask = m_nextWaitingTask;
		m_nextWaitingTask = task;
		m_condVarTasks.notify_one();
	}
	else
#endif
	{
		task->m_nextWaitingTask = m_nextWaitingTask;
		m_nextWaitingTask = task;
	}
}

void hxTaskQueue::waitForAll() {
#if HX_HAS_CPP11_THREADS
	if (m_threadPoolSize > 0) {
		// Contribute current thread and request waiting until completion.
		executorThread(this, ExecutorMode::Waiting);
	}
	else
#endif
	{
		while (m_nextWaitingTask) {
			Task* task = m_nextWaitingTask;
			m_nextWaitingTask = task->m_nextWaitingTask;
			task->m_queue = null;
			task->m_nextWaitingTask = null;

			// Last time this object is touched.  It may delete or re-enqueue itself.
			hxProfileScope(task->getLabel());
			task->execute(this);
		}
	}
}

#if HX_HAS_CPP11_THREADS
void hxTaskQueue::executorThread(hxTaskQueue* q, ExecutorMode mode) {
	Task* task = null;
	for (;;) {
		{
			std::unique_lock<std::mutex> lk(q->m_mutex);

			if (task) {
				// Waited to reacquire critical section to decrement counter for previous task.
				task = null;
				hxAssert(q->m_executingCount > 0);
				if (--q->m_executingCount == 0 && !q->m_nextWaitingTask) {
					q->m_condVarWaiting.notify_all();
				}
			}

			// Either aquire a next task or meet stopping criteria.
			if (mode == ExecutorMode::Pool) {
				q->m_condVarTasks.wait(lk, [q] {
					return q->m_nextWaitingTask || q->m_runningQueueCheck != c_runningQueueCheck;
				});
			}

			if (q->m_nextWaitingTask) {
				hxAssert(q->m_runningQueueCheck == c_runningQueueCheck);
				task = q->m_nextWaitingTask;
				q->m_nextWaitingTask = task->m_nextWaitingTask;
				++q->m_executingCount;
			}
			else {
				if (mode != ExecutorMode::Pool) {
					q->m_condVarWaiting.wait(lk, [q] {
						return q->m_executingCount == 0 && !q->m_nextWaitingTask;
					});

					if (mode == ExecutorMode::Stopping) {
						hxAssert(q->m_runningQueueCheck == c_runningQueueCheck);
						q->m_runningQueueCheck = 0u;
						q->m_condVarTasks.notify_all();
					}
				}

				return;
			}
		}

		task->m_nextWaitingTask = null;
		task->m_queue = null;
		hxProfileScope(task->getLabel());
		// Last time this object is touched.  It may delete or re-enqueue itself.
		task->execute(q);
	}
}
#endif
