#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include "hatchling.h"
#include "hxtask.hpp"
#include "hxthread.hpp"

/// `hxtask_queue` - Provides a simple task queue with a worker thread pool.
/// Implements single threaded task queuing when `HX_USE_THREADS=0`. Executes
/// supplied tasks in arbitrary order without cancellation using an optional
/// thread pool. Use a separate task graph manager to generate tasks if that is
/// needed. See `<hx/hxtask.hpp>`.
class hxtask_queue {
public:
	/// Create a new task queue. `thread_pool_size` determines the size of the
	/// worker thread pool. A thread_pool_size of `0` does not use threads.
	explicit hxtask_queue(size_t thread_pool_size_);

	/// Calls wait_for_all before destructing.
	~hxtask_queue(void);

	/// Queue a task for later execution. Does not delete task after execution.
	/// Thread safe and callable from running tasks.
	/// - `task` : A pointer to the task to be enqueued for execution.
	void enqueue(hxtask* task_) hxattr_nonnull(2);

	/// The thread calling `wait_for_all` will execute tasks as well. Do not call
	/// from `hxtask::execute`.
	void wait_for_all(void);

private:
	hxtask_queue(const hxtask_queue&) = delete;
	void operator=(const hxtask_queue&) = delete;

	hxtask* m_next_task_;

#if HX_USE_THREADS
	friend class hxtask_wait_for_tasks_;
	friend class hxtask_wait_for_completion_;

	enum thread_mode_t_ : uint8_t {
		thread_mode_pool_,
		thread_mode_waiting_,
		thread_mode_stopping_
	};
	enum run_level_t_ : uint32_t {
		run_level_running_ = (uint32_t)0x00c0ffee,
		run_level_stopped_ = (uint32_t)0xdeadbeef
	};

	static void* thread_task_loop_entry_(hxtask_queue* q_);
	static void thread_task_loop_(hxtask_queue* q_, thread_mode_t_ mode_);

	run_level_t_ m_queue_run_level_;
	size_t m_thread_pool_size_;
	hxthread* m_threads_;
	hxmutex m_mutex_;
	hxcondition_variable m_cond_var_new_tasks_;
	hxcondition_variable m_cond_var_completion_;
	int32_t m_executing_count_;
#endif
};
