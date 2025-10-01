// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include "../include/hx/hxtask_queue.hpp"
#include "../include/hx/hxprofiler.hpp"

HX_REGISTER_FILENAME_HASH

#if HX_USE_THREADS
// hxtask_wait_for_tasks_ - This is a worker task waiting for tasks or shutdown.
class hxtask_wait_for_tasks_ {
public:
	hxtask_wait_for_tasks_(hxtask_queue* q) : q_(q) {}
	bool operator()(void) const {
		return q_->m_next_task_
			|| q_->m_queue_run_level_ == hxtask_queue::run_level_stopped_;
	}
	hxtask_queue* q_;
};

// hxtask_wait_for_completion_ - This is a task waiting on all task completion
// and possibly waiting to shutdown the queue as well. Neither wait states should
// occur when the queue has already started shutting down.
class hxtask_wait_for_completion_ {
public:
	hxtask_wait_for_completion_(hxtask_queue* q) : q_(q) {}
	bool operator()(void) const {
		hxassertmsg(q_->m_queue_run_level_ == hxtask_queue::run_level_running_,
			"threading_error");
		return q_->m_next_task_ == hxnull && q_->m_executing_count_ == 0;
	}
	hxtask_queue* q_;
};
#endif

// Should abort if exceptions are enabled and
hxtask_queue::hxtask_queue(size_t thread_pool_size_)
	: m_next_task_(hxnull)
#if HX_USE_THREADS
	, m_queue_run_level_(run_level_running_)
	, m_thread_pool_size_(thread_pool_size_)
	, m_threads_(hxnull)
	, m_executing_count_(0)
#endif
{
	(void)thread_pool_size_;
#if HX_USE_THREADS
	if(m_thread_pool_size_ > 0) {
		m_threads_ = (hxthread*)hxmalloc(m_thread_pool_size_ * sizeof(hxthread));
		for(size_t i_ = m_thread_pool_size_; i_--;) {
			::new (m_threads_ + i_) hxthread(thread_task_loop_entry_, this);
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

void hxtask_queue::enqueue(hxtask* task_) {
	task_->set_task_queue(this);

#if HX_USE_THREADS
	if(m_thread_pool_size_ > 0) {
		hxunique_lock lock_(m_mutex_);
		hxassertrelease(m_queue_run_level_ == run_level_running_, "stopped_queue");
		task_->set_next_task(m_next_task_);
		m_next_task_ = task_;
		m_cond_var_new_tasks_.notify_one();
	}
	else
#endif
	{
		task_->set_next_task(m_next_task_);
		m_next_task_ = task_;
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
		while(m_next_task_) {
			hxtask* task_ = m_next_task_;
			m_next_task_ = task_->get_next_task();
			task_->set_next_task(hxnull);
			task_->set_task_queue(hxnull);

			// Last time this object is touched. It may delete or re-enqueue
			// itself, we don't care.
			hxprofile_scope(task_->get_label());
			task_->execute(this);
		}
	}
}

#if HX_USE_THREADS
void* hxtask_queue::thread_task_loop_entry_(hxtask_queue* q_) {
	thread_task_loop_(q_, thread_mode_pool_);
	return hxnull;
}

void hxtask_queue::thread_task_loop_(hxtask_queue* q_, thread_mode_t_ mode_) {
	hxtask* task_ = hxnull;
	for(;;) {
		{
			// task is executed outside of this RAII lock.
			hxunique_lock lk_(q_->m_mutex_);

			if(task_) {
				// Waited to reacquire critical after previous task.
				task_ = hxnull;
				hxassertmsg(q_->m_executing_count_ > 0, "internal_error");
				if(--q_->m_executing_count_ == 0 && !q_->m_next_task_) {
					q_->m_cond_var_completion_.notify_all();
				}
			}

			// Workers wait for a next task or run_level_stopped_.
			if(mode_ == thread_mode_pool_) {
				// Use predicate for spurious wakeups
				q_->m_cond_var_new_tasks_.wait(lk_, hxtask_wait_for_tasks_(q_));
			}

			// Waiting threads contribute to the work.
			if(q_->m_next_task_) {
				task_ = q_->m_next_task_;
				q_->m_next_task_ = task_->get_next_task();
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

						// This should trigger a release assert in unexpected waiting threads.
						q_->m_cond_var_completion_.notify_all();
					}
				}
				return;
			}
		}

		task_->set_next_task(hxnull);
		task_->set_task_queue(hxnull);
		hxprofile_scope(task_->get_label());

		// This actually the last time this object is touched. It may delete or
		// re-enqueue itself. The queue is not locked and completion will not
		// be reported until after the task is done.
		task_->execute(q_);
	}
}
#endif
