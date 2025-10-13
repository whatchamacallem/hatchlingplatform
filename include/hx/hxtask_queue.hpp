#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

// XXX
// enqueue_after - tasks submitted before will be assigned a thread before ones
// added after. make the scheduler dispatch in order by maintaining a tail
// pointer.
// enqueue_priority - insert by walking past head pointer until priority is less
// than nodes found. assert ordered queue property as well.

#include "hatchling.h"
#include "hxarray.hpp"
#include "hxtask.hpp"
#include "hxthread.hpp"

/// `hxtask_queue` - Provides a simple task queue with a worker thread pool.
/// Implements single-threaded task queuing when `HX_USE_THREADS=0`. Executes
/// supplied tasks in arbitrary order without cancellation using an optional
/// thread pool. Use a separate task-graph manager to generate tasks if needed.
/// See `<hx/hxtask.hpp>`.
class hxtask_queue {
public:
	/// Creates a new task queue. `task_queue_size` reserves storage for enqueued
	/// tasks. `thread_pool_size` determines the size of the worker thread pool.
	/// A `thread_pool_size` of `0` does not use threads.
	explicit hxtask_queue(size_t task_queue_size_, size_t thread_pool_size_);

	/// Calls `wait_for_all` before destruction.
	~hxtask_queue(void);

	/// Queues a task for later execution. Does not delete the task after
	/// execution. Thread-safe and callable from running tasks.
	/// - `task` : A pointer to the task to be enqueued for execution.
	/// - `priority` : Optional priority for scheduling. Higher values run sooner.
	void enqueue(hxtask* task_, int priority_=0) hxattr_nonnull(2);

	/// The thread calling `wait_for_all` executes tasks as well. Do not call from
	/// `hxtask::execute`.
	void wait_for_all(void);

private:
	hxtask_queue(const hxtask_queue&) = delete;
	void operator=(const hxtask_queue&) = delete;

	// This allows examining the state of the scheduler in the watch window.
	struct task_record_t_ {
		hxtask* task_;
		int priority_;
		bool operator<(const task_record_t_& x_) const { return this->priority_ < x_.priority_; }

#if (HX_RELEASE) == 0
		const char* label_;
		~task_record_t_() { ::memset((void*)this, 0x00, sizeof *this); }
#endif
	};

	hxarray<task_record_t_> m_tasks_;

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
