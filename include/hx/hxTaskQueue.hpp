#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>
#include <hx/hxTask.hpp>

#if HX_USE_CPP_THREADS
#include <mutex>
#include <condition_variable>
#include <thread>
#endif

// ----------------------------------------------------------------------------
// hxTaskQueue - Execute supplied tasks in arbitrary order without cancellation
// using an optional thread pool. See <hx/hxTask.hpp>.
class hxTaskQueue {
public:
	// Create a new task queue. threadPoolSize determines the size of the worker
	// pool. A threadPoolSize of -1 indicates using a hardware_concurrency()-1 size
	// thread pool. A threadPoolSize of 0 does not use threading.
    explicit hxTaskQueue(int32_t threadPoolSize_ = -1);

	// Calls waitForAll before destructing.
    ~hxTaskQueue();

	// Queue a task for later execution. Does not delete task after execution.
	// Thread safe and callable from running tasks.
    // - task_: A pointer to the task to be enqueued for execution.
    void enqueue(hxTask* task_);

	// The thread calling waitForAll() will execute tasks as well. Do not call
	// from hxTask::execute().
    void waitForAll();

private:
    hxTaskQueue(const hxTaskQueue&) HX_DELETE_FN;
    void operator=(const hxTaskQueue&) HX_DELETE_FN;

    static const uint32_t RunningQueueCheck_ = 0xc710b034u;

    hxTask* m_nextTask;
    uint32_t m_runningQueueCheck;

#if HX_USE_CPP_THREADS
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
