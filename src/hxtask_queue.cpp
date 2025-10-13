// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include "../include/hx/hxtask_queue.hpp"
#include "../include/hx/hxprofiler.hpp"

HX_REGISTER_FILENAME_HASH

#if HX_USE_THREADS
// hxtask_wait_for_tasks_ keeps worker threads waiting for tasks or shutdown.
class hxtask_wait_for_tasks_ {
public:
	hxtask_wait_for_tasks_(hxtask_queue* q) : q_(q) {}
	bool operator()(void) const {
		return !q_->m_tasks_.empty()
			|| q_->m_queue_run_level_ == hxtask_queue::run_level_stopped_;
	}
	hxtask_queue* q_;
};

// hxtask_wait_for_completion_ waits for all work to complete and may also wait
// to shut down the queue. Neither wait state should occur after shutdown has
// started.
class hxtask_wait_for_completion_ {
public:
	hxtask_wait_for_completion_(hxtask_queue* q) : q_(q) {}
	bool operator()(void) const {
		hxassertmsg(q_->m_queue_run_level_ == hxtask_queue::run_level_running_,
			"threading_error");
		return q_->m_tasks_.empty() && q_->m_executing_count_ == 0;
	}
	hxtask_queue* q_;
};
#endif

// Should abort if exceptions are enabled and the thread pool cannot be created.
hxtask_queue::hxtask_queue(size_t task_queue_size_, size_t thread_pool_size_)
#if HX_USE_THREADS
	: m_queue_run_level_(run_level_running_)
	, m_thread_pool_size_(thread_pool_size_)
	, m_threads_(hxnull)
	, m_executing_count_(0)
#endif
{
	m_tasks_.reserve(task_queue_size_);

	(void)thread_pool_size_;
#if HX_USE_THREADS
	if(m_thread_pool_size_ > 0) {
		m_threads_ = (hxthread*)hxmalloc(m_thread_pool_size_ * sizeof(hxthread));
		for(size_t i_ = m_thread_pool_size_; i_--;) {
			::new(m_threads_ + i_) hxthread(thread_task_loop_entry_, this);
		}
	}
#endif
}

hxtask_queue::~hxtask_queue(void) {
#if HX_USE_THREADS
	if(m_thread_pool_size_ > 0) {
		thread_task_loop_(this, thread_mode_stopping_);
		hxassertmsg(m_queue_run_level_ == run_level_stopped_, "threading_error");

		for(size_t i_ = m_thread_pool_size_; i_--;) {
			m_threads_[i_].join();
			m_threads_[i_].~hxthread();
		}
		hxfree(m_threads_);
		m_threads_ = hxnull;
	}
	else
#endif
	{
		wait_for_all();
	}
}

void hxtask_queue::enqueue(hxtask* task, int priority) {
	task_record_t entry = { task, priority
#if (HX_RELEASE) == 0
		, task->get_label()
#endif
	};

#if HX_USE_THREADS
	if(m_thread_pool_size_ > 0) {
		hxunique_lock lock_(m_mutex_);
		hxassertrelease(m_queue_run_level_ == run_level_running_, "stopped_queue");
		m_tasks_.push_heap(entry);
		m_cond_var_new_tasks_.notify_one();
	}
	else
#endif
	{
		m_tasks_.push_heap(entry);
	}
}

void hxtask_queue::wait_for_all(void) {
#if HX_USE_THREADS
	if(m_thread_pool_size_ > 0) {
		// Contribute current thread and request waiting until completion.
		thread_task_loop_(this, thread_mode_waiting_);
	}
	else
#endif
	{
		while(!m_tasks_.empty()) {
			hxtask* task = m_tasks_.front().task;
			m_tasks_.pop_heap();

			// This is the last time this object is touched. It may delete or
			// re-enqueue itself; we don't care.
			hxprofile_scope(task->get_label());
			task->execute(this);
		}
	}
}

#if HX_USE_THREADS
void* hxtask_queue::thread_task_loop_entry_(hxtask_queue* q_) {
	thread_task_loop_(q_, thread_mode_pool_);
	return hxnull;
}

void hxtask_queue::thread_task_loop_(hxtask_queue* q_, thread_mode_t_ mode_) {
	hxtask* task = hxnull;
	for(;;) {
		{
			// The task executes outside of this RAII lock.
			hxunique_lock lk_(q_->m_mutex_);

			if(task) {
				// Finished reacquiring the critical section after the previous task.
				task = hxnull;
				hxassertmsg(q_->m_executing_count_ > 0, "internal_error");
				if(--q_->m_executing_count_ == 0 && q_->m_tasks_.empty()) {
					q_->m_cond_var_completion_.notify_all();
				}
			}

			// Workers wait for a next task or run_level_stopped_.
			if(mode_ == thread_mode_pool_) {
				// Use a predicate to handle spurious wakeups.
				q_->m_cond_var_new_tasks_.wait(lk_, hxtask_wait_for_tasks_(q_));
			}

			// Waiting threads contribute to the work.
			if(!q_->m_tasks_.empty()) {
				task = q_->m_tasks_.front().task;
				q_->m_tasks_.pop_heap();
				++q_->m_executing_count_;
			}
			else {
				// Nothing left for worker threads to do. The waiting threads still
				// have work to do before they leave the main task loop.

				if(mode_ != thread_mode_pool_) {
					// All tasks are dispatched. Now wait for m_executing_count_ to hit 0.
					// Tasks may enqueue subtasks before processing is considered done.
					// This asserts the queue is still running.
					q_->m_cond_var_completion_.wait(lk_, hxtask_wait_for_completion_(q_));

					// All tasks are now considered complete. The workers can be
					// released if the queue is shutting down.
					if(mode_ == thread_mode_stopping_) {
						q_->m_queue_run_level_ = run_level_stopped_;
						q_->m_cond_var_new_tasks_.notify_all();

					// This triggers a release assert in any unexpected waiting threads.
					q_->m_cond_var_completion_.notify_all();
					}
				}
				return;
			}
		}

		hxprofile_scope(task->get_label());

		// This is actually the last time this object is touched. It may delete or
		// re-enqueue itself. The queue is not locked and completion is not reported
		// until after the task is done.
		task->execute(q_);
	}
}
#endif
