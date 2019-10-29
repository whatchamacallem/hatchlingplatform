#pragma once
// Copyright 2017 Adrian Johnston

#include "hatchling.h"

#if HX_HAS_CPP11_THREADS
#include <mutex>
#include <condition_variable>
#include <thread>
#endif

// ----------------------------------------------------------------------------
// hxTaskQueue.  Execute supplied tasks in arbitrary order without cancellation
// using an optional thread pool.

class hxTaskQueue {
public:
	// Base class for tasks to be queued.
	class Task {
	public:
		// staticLabel must be a static string.
		explicit Task(const char* staticLabel = "task") : m_queue(hx_null), m_nextWaitingTask(hx_null), m_label(staticLabel) { }

		// Not called by queue. execute() may delete this _if allocator is thread safe_.
		virtual ~Task() { hxAssertRelease(!m_queue, "deleting queued task: %s", getLabel()); }

		// Will be wrapped in hxProfileScope(getLabel());
		virtual void execute(hxTaskQueue* q) = 0;

		const char* getLabel() const { return m_label; }
		void setLabel(const char* x) { m_label = x; }

	private:
		friend class hxTaskQueue;

		Task(const Task&); // = delete
		void operator=(const Task&); // = delete

		hxTaskQueue* m_queue;
		Task* m_nextWaitingTask;
		const char* m_label;
	};

	// threadPoolSize -1 indicates using a hardware_concurrency()-1 size thread pool.
	// threadPoolSize 0 does not use threading. 
	explicit hxTaskQueue(int32_t threadPoolSize = -1);

	// Calls waitForAll before destructing.
	~hxTaskQueue();

	// Does not delete task after execution.  Thread safe and callable from
	// running tasks.
	void enqueue(Task* task);

	// The thread calling waitForAll() will execute tasks as well.  Do not call
	// from Task::execute().
	void waitForAll();

private:
	hxTaskQueue(const hxTaskQueue&); // = delete
	void operator=(const hxTaskQueue&); // = delete

	static const uint32_t c_runningQueueCheck = 0xc710b034u;

	Task* m_nextWaitingTask;
	uint32_t m_runningQueueCheck;

#if HX_HAS_CPP11_THREADS
	enum class ExecutorMode { Pool, Waiting, Stopping };
	static void executorThread(hxTaskQueue* q, ExecutorMode mode);

	int32_t m_threadPoolSize = 0;
	std::thread* m_threads = hx_null;
	std::mutex m_mutex;
	std::condition_variable m_condVarTasks;
	std::condition_variable m_condVarWaiting;
	int32_t m_executingCount = 0;
#endif
};
