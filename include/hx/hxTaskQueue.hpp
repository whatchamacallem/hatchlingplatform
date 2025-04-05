#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>
#include <hx/hxTask.hpp>

#if HX_USE_CPP11_THREADS
#include <mutex>
#include <condition_variable>
#include <thread>
#endif

// ----------------------------------------------------------------------------
// hxTaskQueue.  Execute supplied tasks in arbitrary order without cancellation
// using an optional thread pool.  See <hx/hxTask.hpp>.

class hxTaskQueue {
public:
	// threadPoolSize -1 indicates using a hardware_concurrency()-1 size thread
	// pool.  threadPoolSize 0 does not use threading. 
	explicit hxTaskQueue(int32_t threadPoolSize_ = -1);

	// Calls waitForAll before destructing.
	~hxTaskQueue();

	// Does not delete task after execution.  Thread safe and callable from
	// running tasks.
	void enqueue(hxTask* task_);

	// The thread calling waitForAll() will execute tasks as well.  Do not call
	// from hxTask::execute().
	void waitForAll();

private:
	hxTaskQueue(const hxTaskQueue&); // = delete
	void operator=(const hxTaskQueue&); // = delete

	static const uint32_t RunningQueueCheck_ = 0xc710b034u;

	hxTask* m_nextTask;
	uint32_t m_runningQueueCheck;

#if HX_USE_CPP11_THREADS
	enum class ExecutorMode_ { Pool_, Waiting_, Stopping_ };
	static void executorThread_(hxTaskQueue* q_, ExecutorMode_ mode_);

	int32_t m_threadPoolSize = 0;
	std::thread* m_threads = hxnull;
	std::mutex m_mutex;
	std::condition_variable m_condVarTasks;
	std::condition_variable m_condVarWaiting;
	int32_t m_executingCount = 0;
#endif
};
