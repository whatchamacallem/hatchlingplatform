#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>
#include <hx/hxtask.hpp>
#include <hx/hxthread.hpp>

// hxtask_queue - Execute supplied tasks in arbitrary order without cancellation
// using an optional thread pool. See <hx/hxtask.hpp>.
class hxtask_queue {
public:
	// Create a new task queue. thread_pool_size determines the size of the worker
	// pool. A thread_pool_size of -1 indicates using a default value, currently 2.
    // A thread_pool_size of 0 does not use threading.
    explicit hxtask_queue(int32_t thread_pool_size_ = -1);

	// Calls wait_for_all before destructing.
    ~hxtask_queue(void);

	// Queue a task for later execution. Does not delete task after execution.
	// Thread safe and callable from running tasks.
    // - task: A pointer to the task to be enqueued for execution.
    void enqueue(hxtask* task_);

	// The thread calling wait_for_all() will execute tasks as well. Do not call
	// from hxtask::execute().
    void wait_for_all(void);

private:
    hxtask_queue(const hxtask_queue&) hxdelete_fn;
    void operator=(const hxtask_queue&) hxdelete_fn;

    friend class hxtask_wait_for_tasks_;
    friend class hxtask_wait_for_completion_;

    hxtask* m_next_task_;

#if HX_USE_THREADS
    enum thread_mode_t_ {
        thread_mode_pool_,
        thread_mode_waiting_,
        thread_mode_stopping_
    };
    enum run_level_t_ {
        run_level_running_ = (uint32_t)0x00c0ffee,
        run_level_stopped_ = (uint32_t)0xdeadbeef
    };

    static void* thread_task_loop_entry_(hxtask_queue* q_);
    static void thread_task_loop_(hxtask_queue* q_, thread_mode_t_ mode_);

    run_level_t_ m_queue_run_level_;
    int32_t m_thread_pool_size_;
    hxthread* m_threads_;
    hxmutex m_mutex_;
    hxcondition_variable m_cond_var_new_tasks_;
    hxcondition_variable m_cond_var_completion_;
    int32_t m_executing_count_;
#endif
};
