#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>
#include <hx/hxtask.hpp>

#if HX_USE_CPP_THREADS
#include <mutex>
#include <condition_variable>
#include <thread>
#endif

// hxtask_queue - Execute supplied tasks in arbitrary order without cancellation
// using an optional thread pool. See <hx/hxtask.hpp>.
class hxtask_queue {
public:
	// Create a new task queue. thread_pool_size determines the size of the worker
	// pool. A thread_pool_size of -1 indicates using a hardware_concurrency()-1 size
	// thread pool. A thread_pool_size of 0 does not use threading.
    explicit hxtask_queue(int32_t thread_pool_size_ = -1);

	// Calls wait_for_all before destructing.
    ~hxtask_queue();

	// Queue a task for later execution. Does not delete task after execution.
	// Thread safe and callable from running tasks.
    // - task: A pointer to the task to be enqueued for execution.
    void enqueue(hxtask* task_);

	// The thread calling wait_for_all() will execute tasks as well. Do not call
	// from hxtask::execute().
    void wait_for_all();

private:
    hxtask_queue(const hxtask_queue&) HX_DELETE_FN;
    void operator=(const hxtask_queue&) HX_DELETE_FN;

    static const uint32_t Running_queue_check_ = 0xc710b034u;

    hxtask* m_next_task_;
    uint32_t m_running_queue_check_;

#if HX_USE_CPP_THREADS
    enum class Executor_mode_ { Pool_, Waiting_, Stopping_ };
    static void executor_thread_(hxtask_queue* q_, Executor_mode_ mode_);

    int32_t m_thread_pool_size_ = 0;
    std::thread* m_threads_ = hxnull;
    std::mutex m_mutex_;
    std::condition_variable m_cond_var_tasks_;
    std::condition_variable m_cond_var_waiting_;
    int32_t m_executing_count_ = 0;
#endif
};
