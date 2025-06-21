#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>
#include <pthread.h>

// hxthread.hpp - Threading Primitives for Hatchling Platform
// This header provides lightweight C++ wrappers around POSIX pthreads for
// thread synchronization and management. The following classes are defined:
//
// - hxmutex
//     Mutex wrapper for pthreads. Provides lock/unlock functionality, error
//     tracking, and ensures proper initialization and destruction. Not copyable.
//
// - hxunique_lock
//     RAII-style unique lock for hxmutex. Locks the mutex on construction and
//     unlocks on destruction. Supports deferred locking and ownership checks.
//     Not copyable.
//
// - hxcondition_variable
//     Condition variable wrapper for pthreads. Allows threads to wait for
//     notifications, supports predicate-based waiting, and provides notify_one
//     and notify_all methods. Not copyable.
//
// - hxthread
//     Thread wrapper for pthreads. Provides thread creation, joining, and
//     detaching. Ensures threads are not left joinable on destruction. Not copyable.


// Mutex wrapper for pthreads.
// Provides lock/unlock functionality and error tracking.
class hxmutex {
public:
    // Constructs a mutex and initializes it.
    hxmutex() : m_last_error_(0), m_valid_(false) {
        m_last_error_ = pthread_mutex_init(&m_mutex_, 0);
        if (m_last_error_ == 0) m_valid_ = true;
    }

    // Destroys the mutex if valid.
    ~hxmutex() {
        if (m_valid_) {
            pthread_mutex_destroy(&m_mutex_);
        }
    }

    // Locks the mutex.
    // Returns true on success, false otherwise.
    bool lock() {
        if (!m_valid_) return false;
        m_last_error_ = pthread_mutex_lock(&m_mutex_);
        return m_last_error_ == 0;
    }

    // Unlocks the mutex.
    // Returns true on success, false otherwise.
    bool unlock() {
        if (!m_valid_) return false;
        m_last_error_ = pthread_mutex_unlock(&m_mutex_);
        return m_last_error_ == 0;
    }

    // Returns a pointer to the native pthread mutex handle.
    pthread_mutex_t* native_handle() { return &m_mutex_; }

    // Returns the last error code from a mutex operation.
    int last_error() const { return m_last_error_; }

    // Returns true if the mutex was successfully initialized.
    bool valid() const { return m_valid_; }

private:
    // Deleted copy constructor.
    hxmutex(const hxmutex&) hxdelete_fn;
    // Deleted copy assignment operator.
    hxmutex& operator=(const hxmutex&) hxdelete_fn;

	pthread_mutex_t m_mutex_;
    int m_last_error_;
    bool m_valid_;
};

// RAII-style unique lock for hxmutex.
// Locks the mutex on construction and unlocks on destruction.
class hxunique_lock {
public:
    // Constructs and locks the given mutex.
    explicit hxunique_lock(hxmutex& m_) : m_mutex_(m_), m_owns_(false) {
        lock();
    }
    // Constructs with option to defer locking.
    // - defer_lock: If true, does not lock the mutex immediately.
    hxunique_lock(hxmutex& m_, bool defer_lock_) : m_mutex_(m_), m_owns_(false) {
        if (!defer_lock_) lock();
    }
    // Unlocks the mutex if owned.
    ~hxunique_lock() {
        if (m_owns_) unlock();
    }
    // Locks the mutex if not already locked.
    void lock() {
        if (!m_owns_) m_owns_ = m_mutex_.lock();
    }
    // Unlocks the mutex if owned.
    void unlock() {
        if (m_owns_) {
            m_mutex_.unlock();
            m_owns_ = false;
        }
    }
    // Returns true if the lock owns the mutex.
    bool owns_lock() const { return m_owns_; }
    // Returns a reference to the associated mutex.
    hxmutex& mutex() { return m_mutex_; }

private:
    // Deleted copy constructor.
    hxunique_lock(const hxunique_lock&) hxdelete_fn;
    // Deleted copy assignment operator.
    hxunique_lock& operator=(const hxunique_lock&) hxdelete_fn;
    hxmutex& m_mutex_;
    bool m_owns_;
};

// Condition variable wrapper for pthreads.
// Allows threads to wait for notifications.
class hxcondition_variable {
public:
    // Constructs and initializes the condition variable.
    hxcondition_variable() : m_last_error_(0), m_valid_(false) {
        m_last_error_ = pthread_cond_init(&m_cond_, 0);
        if (m_last_error_ == 0) { m_valid_ = true; }
    }

    // Destroys the condition variable if valid.
    ~hxcondition_variable() {
        if (m_valid_) {
            pthread_cond_destroy(&m_cond_);
        }
    }

    // Waits for the condition variable to be notified.
    // - mutex: The mutex to use for waiting.
    // Returns true on success, false otherwise.
    bool wait(hxmutex& mutex_) {
        if (!m_valid_ || !mutex_.valid()) return false;
        m_last_error_ = pthread_cond_wait(&m_cond_, mutex_.native_handle());
        return m_last_error_ == 0;
    }

    // Overload: Waits using a hxunique_lock.
    // - lock: The unique lock to use for waiting.
    // Returns true on success, false otherwise.
    bool wait(hxunique_lock& lock_) {
        return wait(lock_.mutex());
    }

    // Waits until the predicate returns true.
    // - lock: The unique lock to use for waiting.
    // - pred: Predicate function to check.
    template<typename predicate_t_>
    void wait(hxunique_lock& lock_, predicate_t_ pred_) {
        while (!pred_()) {
            wait(lock_);
        }
    }

    // Notifies one waiting thread.
    // Returns true on success, false otherwise.
    bool notify_one() {
        if (!m_valid_) return false;
        m_last_error_ = pthread_cond_signal(&m_cond_);
        return m_last_error_ == 0;
    }

    // Notifies all waiting threads.
    // Returns true on success, false otherwise.
    bool notify_all() {
        if (!m_valid_) return false;
        m_last_error_ = pthread_cond_broadcast(&m_cond_);
        return m_last_error_ == 0;
    }

    // Returns a pointer to the native pthread condition variable handle.
    pthread_cond_t* native_handle() { return &m_cond_; }

    // Returns the last error code from a condition variable operation.
    int last_error() const { return m_last_error_; }

    // Returns true if the condition variable was successfully initialized.
    bool valid() const { return m_valid_; }

private:
    // Deleted copy constructor.
    hxcondition_variable(const hxcondition_variable&) hxdelete_fn;
    // Deleted copy assignment operator.
    hxcondition_variable& operator=(const hxcondition_variable&) hxdelete_fn;

	pthread_cond_t m_cond_;
    int m_last_error_;
    bool m_valid_;
};

// Thread wrapper for pthreads.
// Provides thread creation, joining, and detaching.
class hxthread {
public:
    typedef void* (*thread_func_t)(void*);
    typedef pthread_t native_handle_type;

    // Default constructor. Thread is not started.
    hxthread() : m_started_(false), m_joined_(false), m_func_(0), m_arg_(0) {}

    // Constructs and starts a thread with the given function and argument.
    // - f: Thread function.
    // - arg: Argument to pass to the thread function.
    // Does not free arg.
    explicit hxthread(thread_func_t f_, void* arg_)
            : m_started_(false), m_joined_(false), m_func_(0), m_arg_(0) {
        start(f_, arg_);
    }

    // Destructor. Asserts that the thread is not joinable.
    ~hxthread() {
		hxassertrelease(!joinable(), "threading error");
    }

    // Starts the thread with the given function and argument.
    // - f: Thread function.
    // - arg: Argument to pass to the thread function.
    void start(thread_func_t f_, void* arg_) {
        hxassertrelease(!joinable(), "threading error");
        m_func_ = f_;
        m_arg_ = arg_;
        int res_ = pthread_create(&m_thread_, 0, m_func_, m_arg_);
        hxassertrelease(res_ == 0, "threading error"); (void)res_;
        m_started_ = true;
        m_joined_ = false;
    }

    // Returns true if the thread has been started and not yet joined or detached.
    bool joinable() const { return m_started_ && !m_joined_; }

    // Joins the thread. Blocks until the thread finishes.
    void join() {
        hxassertrelease(joinable(), "threading error");
        pthread_join(m_thread_, 0);
        m_joined_ = true;
    }

    // Detaches the thread, allowing it to run independently.
    void detach() {
        hxassertrelease(joinable(), "threading error");
        pthread_detach(m_thread_);
        m_joined_ = true;
    }

    // Returns the native pthread thread handle.
    native_handle_type native_handle() { return m_thread_; }

private:
    // Deleted copy constructor.
    hxthread(const hxthread&) hxdelete_fn;
    // Deleted copy assignment operator.
    hxthread& operator=(const hxthread&) hxdelete_fn;

    pthread_t m_thread_;
    bool m_started_;
    bool m_joined_;
    void* (*m_func_)(void*);
    void* m_arg_;
};
