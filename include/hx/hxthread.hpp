#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>
#include <pthread.h>

// hxthread.hpp - Threading primitives that mostly adhere to the C++ standard.
// This header provides lightweight C++ wrappers around POSIX pthreads for
// thread synchronization and management. The following classes are defined:
//
// - hxmutex
//     Mutex wrapper for pthreads. Provides lock/unlock functionality, error
//     tracking, and ensures proper initialization and destruction. Not copyable.
//     Asserts in debug and uses 2 bytes to track validity and last pthread error
//     code otherwise.
//
// - hxunique_lock
//     RAII-style unique lock for hxmutex. Locks the mutex on construction and
//     unlocks on destruction. Supports deferred locking and ownership checks.
//     Not copyable.
//
// - hxcondition_variable
//     Condition variable wrapper for pthreads. Allows threads to wait for
//     notifications, supports predicate-based waiting, and provides notify_one
//     and notify_all methods. Not copyable. Asserts in debug and uses 2 bytes
//     to track validity and last pthread error code.
//
// - hxthread
//     Thread wrapper for pthreads. Provides thread creation, joining, and
//     detaching. Ensures threads are not left joinable on destruction. Not copyable.
//     Errors are threated as release mode asserts instead of being tracked.


// hxmutex - std::mutex style wrapper for pthreads. Asserts on unexpected failure
// by the posix api.
class hxmutex {
public:
    // Constructs a mutex and initializes it.
    inline hxmutex() : m_last_error_(0), m_valid_(false) {
        int last_error_ = pthread_mutex_init(&m_mutex_, 0);
        m_last_error_ = (unsigned char)last_error_;
        m_valid_ = last_error_ == 0;
        hxassertmsg(last_error_ == 0, "pthread_mutex_init %d", last_error_);
    }

    // Destroys the mutex if valid.
    inline ~hxmutex() {
        if (m_valid_) {
            int last_error_ = pthread_mutex_destroy(&m_mutex_);
            hxassertmsg(last_error_ == 0, "pthread_mutex_destroy %d",last_error_ ); (void)last_error_;
        }
    }

    // Locks the mutex.
    // Returns true on success, false otherwise.
    inline bool lock() {
        if (!m_valid_) return false;
        int last_error_ = pthread_mutex_lock(&m_mutex_);
        m_last_error_ = (unsigned char)last_error_;
        hxassertmsg(last_error_ == 0, "pthread_mutex_lock %d", last_error_);
        return last_error_ == 0;
    }

    // Unlocks the mutex.
    // Returns true on success, false otherwise.
    inline bool unlock() {
        if (!m_valid_) return false;
        int last_error_ = pthread_mutex_unlock(&m_mutex_);
        m_last_error_ = (unsigned char)last_error_;
        hxassertmsg(last_error_ == 0, "pthread_mutex_unlock %d", last_error_);
        return last_error_ == 0;
    }

    // Returns a pointer to the native pthread mutex handle.
    inline pthread_mutex_t* native_handle() { return &m_mutex_; }

    // Returns the last error code from a mutex operation. Assumes the code
    // was less than 255.
    inline int last_error() const { return (int)m_last_error_; }

    // Returns true if the mutex was successfully initialized.
    inline bool valid() const { return m_valid_; }

private:
    // Deleted copy constructor.
    hxmutex(const hxmutex&) hxdelete_fn;
    // Deleted copy assignment operator.
    hxmutex& operator=(const hxmutex&) hxdelete_fn;

	pthread_mutex_t m_mutex_;
    // 2 bytes overhead for debugging.
    unsigned char m_last_error_  ;
    bool m_valid_;
};

// hxunique_lock - std::unique_lock style RAII-style unique lock for hxmutex.
// Locks the mutex on construction and unlocks on destruction.
class hxunique_lock {
public:
    // Constructs and locks the given mutex.
    inline explicit hxunique_lock(hxmutex& m_)
            : m_mutex_(m_), m_owns_(false) {
        lock();
    }
    // Constructs with option to defer locking.
    // - defer_lock: If true, does not lock the mutex immediately.
    inline hxunique_lock(hxmutex& m_, bool defer_lock_)
            : m_mutex_(m_), m_owns_(false) {
        if (!defer_lock_) lock();
    }
    // Unlocks the mutex if owned.
    inline ~hxunique_lock() {
        if (m_owns_) unlock();
    }
    // Locks the mutex if not already locked.
    inline void lock() {
        if (!m_owns_) m_owns_ = m_mutex_.lock();
    }
    // Unlocks the mutex if owned.
    inline void unlock() {
        if (m_owns_) {
            m_mutex_.unlock();
            m_owns_ = false;
        }
    }
    // Returns true if the lock owns the mutex.
    inline bool owns_lock() const { return m_owns_; }
    // Returns a reference to the associated mutex.
    inline hxmutex& mutex() { return m_mutex_; }

private:
    // Deleted copy constructor.
    hxunique_lock(const hxunique_lock&) hxdelete_fn;
    // Deleted copy assignment operator.
    hxunique_lock& operator=(const hxunique_lock&) hxdelete_fn;
    hxmutex& m_mutex_;
    bool m_owns_;
};

// hxcondition_variable - std::condition_variable style condition variable
// wrapper for pthreads. Allows threads to wait for notifications.
class hxcondition_variable {
public:
    // Constructs and initializes the condition variable.
    inline hxcondition_variable() : m_last_error_(0), m_valid_(false) {
        int last_error_ = pthread_cond_init(&m_cond_, 0);
        m_last_error_ = (unsigned char)last_error_;
        m_valid_ = last_error_ == 0;
        hxassertmsg(last_error_ == 0, "pthread_cond_init %d", last_error_);
    }

    // Destroys the condition variable if valid.
    inline ~hxcondition_variable() {
        if (m_valid_) {
            int last_error_ = pthread_cond_destroy(&m_cond_);
            hxassertmsg(last_error_ == 0, "pthread_cond_destroy %d", last_error_); (void)last_error_;
        }
    }

    // Waits for the condition variable to be notified.
    // - mutex: The mutex to use for waiting.
    // Returns true on success, false otherwise.
    inline bool wait(hxmutex& mutex_) {
        if (!m_valid_ || !mutex_.valid()) { return false; };

        int last_error_ = pthread_cond_wait(&m_cond_, mutex_.native_handle());
        m_last_error_ = (unsigned char)last_error_;
        hxassertmsg(last_error_ == 0, "pthread_cond_init %d", last_error_);
        return last_error_ == 0;
    }

    // Overload: Waits using a hxunique_lock.
    // - lock: The unique lock to use for waiting.
    // Returns true on success, false otherwise.
    inline bool wait(hxunique_lock& lock_) {
        return wait(lock_.mutex());
    }

    // Waits until the predicate returns true.
    // - lock: The unique lock to use for waiting.
    // - pred: Predicate function to check.
    template<typename predicate_t_>
    inline void wait(hxunique_lock& lock_, predicate_t_ pred_) {
        while (!pred_()) {
            wait(lock_);
        }
    }

    // Notifies one waiting thread.
    // Returns true on success, false otherwise.
    inline bool notify_one() {
        if (!m_valid_) { return false; }
        int last_error_ = pthread_cond_signal(&m_cond_);
        m_last_error_ = (unsigned char)last_error_;
        hxassertmsg(last_error_ == 0, "pthread_cond_init %d", last_error_);
        return last_error_ == 0;
    }

    // Notifies all waiting threads.
    // Returns true on success, false otherwise.
    inline bool notify_all() {
        if (!m_valid_) { return false; }
        int last_error_ = pthread_cond_broadcast(&m_cond_);
        m_last_error_ = (unsigned char)last_error_;
        hxassertmsg(last_error_ == 0, "pthread_cond_init %d", last_error_);
        return last_error_ == 0;
    }

    // Returns a pointer to the native pthread condition variable handle.
    inline pthread_cond_t* native_handle() { return &m_cond_; }

    // Returns the last error code from a condition variable operation.
    // Assumes the code was less than 255.
    inline int last_error() const { return m_last_error_; }

    // Returns true if the condition variable was successfully initialized.
    inline bool valid() const { return m_valid_; }

private:
    // Deleted copy constructor.
    hxcondition_variable(const hxcondition_variable&) hxdelete_fn;
    // Deleted copy assignment operator.
    hxcondition_variable& operator=(const hxcondition_variable&) hxdelete_fn;

	pthread_cond_t m_cond_;
    unsigned char m_last_error_  ;
    bool m_valid_;
};

// hxthread - std::thread style thread wrapper for pthreads. Provides thread
// creation, joining, and detaching.
class hxthread {
public:
    typedef void* (*thread_func_t)(void*);
    typedef pthread_t native_handle_type;

    // Default constructor. Thread is not started.
    inline hxthread() : m_started_(false), m_joined_(false), m_func_(0), m_arg_(0) {}

    // Constructs and starts a thread with the given function and argument.
    // - f: Thread function.
    // - arg: Argument to pass to the thread function.
    // Does not free arg.
    inline explicit hxthread(thread_func_t f_, void* arg_)
            : m_started_(false), m_joined_(false), m_func_(0), m_arg_(0) {
        start(f_, arg_);
    }

    // Destructor. Asserts that the thread is not joinable.
    inline ~hxthread() {
		hxassertrelease(!joinable(), "threading error");
    }

    // Starts the thread with the given function and argument.
    // - f: Thread function.
    // - arg: Argument to pass to the thread function.
    inline void start(thread_func_t f_, void* arg_) {
        hxassertrelease(!joinable(), "threading error");
        m_func_ = f_;
        m_arg_ = arg_;
        int res_ = pthread_create(&m_thread_, 0, m_func_, m_arg_);
        hxassertrelease(res_ == 0, "threading error"); (void)res_;
        m_started_ = true;
        m_joined_ = false;
    }

    // Returns true if the thread has been started and not yet joined or detached.
    inline bool joinable() const { return m_started_ && !m_joined_; }

    // Joins the thread. Blocks until the thread finishes.
    inline void join() {
        hxassertrelease(joinable(), "threading error");
        pthread_join(m_thread_, 0);
        m_joined_ = true;
    }

    // Detaches the thread, allowing it to run independently.
    inline void detach() {
        hxassertrelease(joinable(), "threading error");
        pthread_detach(m_thread_);
        m_joined_ = true;
    }

    // Returns the native pthread thread handle.
    inline native_handle_type native_handle() { return m_thread_; }

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
