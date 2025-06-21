// Copyright 2017-2025 Adrian Johnston

#include <hx/hxtask_queue.hpp>
#include <hx/hxprofiler.hpp>

HX_REGISTER_FILENAME_HASH

struct hxwait_pred_tasks_ {
    hxtask_queue* q_;
    hxwait_pred_tasks_(hxtask_queue* q) : q_(q) {}
    bool operator()() const {
        return q_->m_next_task_ || q_->m_running_queue_guard_ != hxtask_queue::running_queue_guard_value_;
    }
};

struct hxwait_pred_waiting_ {
    hxtask_queue* q_;
    hxwait_pred_waiting_(hxtask_queue* q) : q_(q) {}
    bool operator()() const {
        return q_->m_executing_count_ == 0 && !q_->m_next_task_;
    }
};

// Should abort if exceptions are enabled and
hxtask_queue::hxtask_queue(int32_t thread_pool_size_)
    : m_next_task_(hxnull)
    , m_running_queue_guard_(running_queue_guard_value_)
#if HX_USE_THREADS
    , m_thread_pool_size_(0)
    , m_threads_(hxnull)
    , m_executing_count_(0)
#endif
{
    (void)thread_pool_size_;
#if HX_USE_THREADS
    m_thread_pool_size_ = (thread_pool_size_ >= 0) ? thread_pool_size_ : 2;
    if (m_thread_pool_size_ > 0) {
        m_threads_ = (hxthread*)hxmalloc(m_thread_pool_size_ * sizeof(hxthread));
        for (int32_t i_ = m_thread_pool_size_; i_--;) {
            executor_arg_t_* arg = new executor_arg_t_(this, executor_mode_pool_);
            ::new (m_threads_ + i_) hxthread(&executor_thread_entry_, arg);
        }
    }
#endif
}

hxtask_queue::~hxtask_queue() {
#if HX_USE_THREADS
    if (m_thread_pool_size_ > 0) {
        executor_thread_(this, executor_mode_stopping_);
        hxassertrelease(m_running_queue_guard_ == 0u, "Q");

        for (int32_t i_ = m_thread_pool_size_; i_--;) {
            hxassertrelease(m_threads_[i_].joinable(), "threading error");
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
        m_running_queue_guard_ = 0u;
    }
}

void hxtask_queue::enqueue(hxtask* task_) {
    hxassertrelease(task_, "null task");
    task_->set_task_queue(this);

#if HX_USE_THREADS
    if (m_thread_pool_size_ > 0) {
        hxunique_lock lock_(m_mutex_);
        hxassertrelease(m_running_queue_guard_ == running_queue_guard_value_, "enqueue to stopped queue");
        task_->set_next_task(m_next_task_);
        m_next_task_ = task_;
        m_cond_var_tasks_.notify_one();
    }
    else
#endif
    {
        task_->set_next_task(m_next_task_);
        m_next_task_ = task_;
    }
}

void hxtask_queue::wait_for_all() {
#if HX_USE_THREADS
    if (m_thread_pool_size_ > 0) {
        // Contribute current thread and request waiting until completion.
        executor_thread_(this, executor_mode_waiting_);
    }
    else
#endif
    {
        while (m_next_task_) {
            hxtask* task_ = m_next_task_;
            m_next_task_ = task_->get_next_task();
            task_->set_next_task(hxnull);
            task_->set_task_queue(hxnull);

            // Last time this object is touched. It may delete or re-enqueue itself, we
            // don't care.
            hxprofile_scope(task_->get_label());
            task_->execute(this);
        }
    }
}

#if HX_USE_THREADS
void* hxtask_queue::executor_thread_entry_(void* arg_) {
    executor_arg_t_* p = static_cast<executor_arg_t_*>(arg_);
    hxtask_queue* q_ = p->queue_;
    executor_mode_t_ mode_ = p->mode_;
    delete p;
    executor_thread_(q_, mode_);
    return hxnull;
}

void hxtask_queue::executor_thread_(hxtask_queue* q_, executor_mode_t_ mode_) {
    hxtask* task_ = hxnull;
    for (;;) {
        {
            hxunique_lock lk_(q_->m_mutex_);

            // task is executed outside of the above RAII lock.
            if (task_) {
                // Waited to reacquire critical section to decrement counter for previous task.
                task_ = hxnull;
                hxassert(q_->m_executing_count_ > 0);
                if (--q_->m_executing_count_ == 0 && !q_->m_next_task_) {
                    q_->m_cond_var_waiting_.notify_all();
                }
            }

            // Either acquire a next task or meet stopping criteria.
            if (mode_ == executor_mode_pool_) {
                // Use predicate for spurious wakeups
                q_->m_cond_var_tasks_.wait(lk_, hxwait_pred_tasks_(q_));
            }

            if (q_->m_next_task_) {
                hxassertrelease(q_->m_running_queue_guard_ == running_queue_guard_value_, "Q");
                task_ = q_->m_next_task_;
                q_->m_next_task_ = task_->get_next_task();
                ++q_->m_executing_count_;
            }
            else {
                if (mode_ != executor_mode_pool_) {
                    q_->m_cond_var_waiting_.wait(lk_, hxwait_pred_waiting_(q_));

                    if (mode_ == executor_mode_stopping_) {
                        hxassertrelease(q_->m_running_queue_guard_ == running_queue_guard_value_, "Q");
                        q_->m_running_queue_guard_ = 0u;
                        q_->m_cond_var_waiting_.notify_all();
                        q_->m_cond_var_tasks_.notify_all();
                    }
                }

				return;
			}
		}

        task_->set_next_task(hxnull);
        task_->set_task_queue(hxnull);
        hxprofile_scope(task_->get_label());
        // Last time this object is touched. It may delete or re-enqueue itself.
        task_->execute(q_);
    }
}
#endif
