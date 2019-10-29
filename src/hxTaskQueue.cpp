// Copyright 2017-2019 Adrian Johnston

#include <hx/hxTaskQueue.h>
#include <hx/hxProfiler.h>

HX_REGISTER_FILENAME_HASH

// ----------------------------------------------------------------------------
// hxTaskQueue

hxTaskQueue::hxTaskQueue(int32_t threadPoolSize)
	: m_nextTask(hxnull)
	, m_runningQueueCheck(c_runningQueueCheck)

{
	(void)threadPoolSize;
#if HX_USE_CPP11_THREADS
	m_threadPoolSize = (threadPoolSize >= 0) ? threadPoolSize
		: ((int32_t)std::thread::hardware_concurrency() - 1);
	if (m_threadPoolSize > 0) {
		m_threads = (std::thread*)hxMalloc(m_threadPoolSize * sizeof(std::thread));
		for (int32_t i = m_threadPoolSize; i--;) {
			::new (m_threads + i) std::thread(executorThread, this, ExecutorMode::Pool);
		}
	}
#endif
}

hxTaskQueue::~hxTaskQueue() {
#if HX_USE_CPP11_THREADS
	if (m_threadPoolSize > 0) {
		// Contribute current thread, request waiting until completion and signal stopping.
		executorThread(this, ExecutorMode::Stopping);
		hxAssertRelease(m_runningQueueCheck == 0u, "Q");

		for (int32_t i = m_threadPoolSize; i--;) {
			m_threads[i].join();
			m_threads[i].~thread();
		}
		hxFree(m_threads);
		m_threads = hxnull;
	}
	else
#endif
	{
		waitForAll();
		m_runningQueueCheck = 0u;
	}
}

void hxTaskQueue::enqueue(hxTask* task) {
	hxAssert(task);
	task->setOwner(this);

#if HX_USE_CPP11_THREADS
	if (m_threadPoolSize > 0) {
		std::unique_lock<std::mutex> lk(m_mutex);
		hxAssertRelease(m_runningQueueCheck == c_runningQueueCheck, "enqueue to stopped queue");
		task->setNextTask(m_nextTask);
		m_nextTask = task;
		m_condVarTasks.notify_one();
	}
	else
#endif
	{
		task->setNextTask(m_nextTask);
		m_nextTask = task;
	}
}

void hxTaskQueue::waitForAll() {
#if HX_USE_CPP11_THREADS
	if (m_threadPoolSize > 0) {
		// Contribute current thread and request waiting until completion.
		executorThread(this, ExecutorMode::Waiting);
	}
	else
#endif
	{
		while (m_nextTask) {
			hxTask* task = m_nextTask;
			m_nextTask = task->getNextTask();
			task->setNextTask(hxnull);
			task->setOwner(hxnull);

			// Last time this object is touched.  It may delete or re-enqueue itself, we
			// don't care.
			hxProfileScope(task->getLabel());
			task->execute(this);
		}
	}
}

#if HX_USE_CPP11_THREADS
void hxTaskQueue::executorThread(hxTaskQueue* q, ExecutorMode mode) {
	hxTask* task = hxnull;
	for (;;) {
		{
			std::unique_lock<std::mutex> lk(q->m_mutex);

			if (task) {
				// Waited to reacquire critical section to decrement counter for previous task.
				task = hxnull;
				hxAssert(q->m_executingCount > 0);
				if (--q->m_executingCount == 0 && !q->m_nextTask) {
					q->m_condVarWaiting.notify_all();
				}
			}

			// Either aquire a next task or meet stopping criteria.
			if (mode == ExecutorMode::Pool) {
				q->m_condVarTasks.wait(lk, [q] {
					return q->m_nextTask || q->m_runningQueueCheck != c_runningQueueCheck;
				});
			}

			if (q->m_nextTask) {
				hxAssertRelease(q->m_runningQueueCheck == c_runningQueueCheck, "Q");
				task = q->m_nextTask;
				q->m_nextTask = task->getNextTask();
				++q->m_executingCount;
			}
			else {
				if (mode != ExecutorMode::Pool) {
					q->m_condVarWaiting.wait(lk, [q] {
						return q->m_executingCount == 0 && !q->m_nextTask;
					});

					if (mode == ExecutorMode::Stopping) {
						hxAssertRelease(q->m_runningQueueCheck == c_runningQueueCheck, "Q");
						q->m_runningQueueCheck = 0u;
						q->m_condVarTasks.notify_all();
					}
				}

				return;
			}
		}

		task->setNextTask(hxnull);
		task->setOwner(hxnull);
		hxProfileScope(task->getLabel());
		// Last time this object is touched.  It may delete or re-enqueue itself.
		task->execute(q);
	}
}
#endif
