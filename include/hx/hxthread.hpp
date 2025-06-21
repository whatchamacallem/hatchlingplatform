#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>
#include <pthread.h>
#include <cerrno>

class hxmutex {
public:
    hxmutex() : m_last_error_(0), m_valid_(false) {
        m_last_error_ = pthread_mutex_init(&m_mutex_, 0);
        if (m_last_error_ == 0) m_valid_ = true;
    }

    ~hxmutex() {
        if (m_valid_) {
            pthread_mutex_destroy(&m_mutex_);
        }
    }

    bool lock() {
        if (!m_valid_) return false;
        m_last_error_ = pthread_mutex_lock(&m_mutex_);
        return m_last_error_ == 0;
    }

    bool unlock() {
        if (!m_valid_) return false;
        m_last_error_ = pthread_mutex_unlock(&m_mutex_);
        return m_last_error_ == 0;
    }

    pthread_mutex_t* native_handle() { return &m_mutex_; }

    int last_error() const { return m_last_error_; }
    bool valid() const { return m_valid_; }

private:
    hxmutex(const hxmutex&) hxdelete_fn;
    hxmutex& operator=(const hxmutex&) hxdelete_fn;

	pthread_mutex_t m_mutex_;
    int m_last_error_;
    bool m_valid_;
};

// RAII-style unique lock for hxmutex
class hxunique_lock {
public:
    explicit hxunique_lock(hxmutex& m) : m_mutex_(m), m_owns_(false) {
        lock();
    }
    // Deferred lock
    hxunique_lock(hxmutex& m, bool defer_lock) : m_mutex_(m), m_owns_(false) {
        if (!defer_lock) lock();
    }
    ~hxunique_lock() {
        if (m_owns_) unlock();
    }
    void lock() {
        if (!m_owns_) m_owns_ = m_mutex_.lock();
    }
    void unlock() {
        if (m_owns_) {
            m_mutex_.unlock();
            m_owns_ = false;
        }
    }
    bool owns_lock() const { return m_owns_; }
    hxmutex& mutex() { return m_mutex_; }

private:
	hxunique_lock(const hxunique_lock&) hxdelete_fn;
    hxunique_lock& operator=(const hxunique_lock&) hxdelete_fn;
    hxmutex& m_mutex_;
    bool m_owns_;
};

class hxcondition_variable {
public:
    hxcondition_variable() : m_last_error_(0), m_valid_(false) {
        m_last_error_ = pthread_cond_init(&m_cond_, 0);
        if (m_last_error_ == 0) { m_valid_ = true; }
    }

    ~hxcondition_variable() {
        if (m_valid_) {
            pthread_cond_destroy(&m_cond_);
        }
    }

    bool wait(hxmutex& mutex) {
        if (!m_valid_ || !mutex.valid()) return false;
        m_last_error_ = pthread_cond_wait(&m_cond_, mutex.native_handle());
        return m_last_error_ == 0;
    }

    // Overload: wait with hxunique_lock (like std::condition_variable)
    bool wait(hxunique_lock& lock) {
        return wait(lock.mutex());
    }

    // Wait with predicate (like std::condition_variable)
    template<typename Predicate>
    void wait(hxunique_lock& lock, Predicate pred) {
        while (!pred()) {
            wait(lock);
        }
    }

    bool notify_one() {
        if (!m_valid_) return false;
        m_last_error_ = pthread_cond_signal(&m_cond_);
        return m_last_error_ == 0;
    }

    bool notify_all() {
        if (!m_valid_) return false;
        m_last_error_ = pthread_cond_broadcast(&m_cond_);
        return m_last_error_ == 0;
    }

    pthread_cond_t* native_handle() { return &m_cond_; }
    int last_error() const { return m_last_error_; }
    bool valid() const { return m_valid_; }

private:
    hxcondition_variable(const hxcondition_variable&) hxdelete_fn;
    hxcondition_variable& operator=(const hxcondition_variable&) hxdelete_fn;

	pthread_cond_t m_cond_;
    int m_last_error_;
    bool m_valid_;
};

class hxthread {
public:
    typedef pthread_t native_handle_type;

    hxthread() : m_started_(false), m_joined_(false), m_func_(0), m_arg_(0) {}

    typedef void* (*thread_func_t)(void*);
    explicit hxthread(thread_func_t f, void* arg) : m_started_(false), m_joined_(false), m_func_(0), m_arg_(0) {
        start(f, arg);
    }

    ~hxthread() {
		hxassertrelease(!joinable(), "threading error");
    }

    void start(thread_func_t f, void* arg) {
		hxassertrelease(!joinable(), "threading error");
        m_func_ = f;
        m_arg_ = arg;
        int res = pthread_create(&m_thread_, 0, m_func_, m_arg_);
        hxassertrelease(res == 0, "threading error"); (void)res;
        m_started_ = true;
        m_joined_ = false;
    }

    bool joinable() const { return m_started_ && !m_joined_; }

    void join() {
        hxassertrelease(joinable(), "threading error");
        pthread_join(m_thread_, 0);
        m_joined_ = true;
    }

    void detach() {
        hxassertrelease(joinable(), "threading error");
        pthread_detach(m_thread_);
        m_joined_ = true;
    }

    native_handle_type native_handle() { return m_thread_; }

private:
    hxthread(const hxthread&) hxdelete_fn;
    hxthread& operator=(const hxthread&) hxdelete_fn;

    pthread_t m_thread_;
    bool m_started_;
    bool m_joined_;
    void* (*m_func_)(void*);
    void* m_arg_;
};
