#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>

#if HX_USE_THREADS
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
//     to track validity and last pthread error code otherwise.
//
// - hxthread
//     Thread wrapper for pthreads. Provides thread creation, joining, and
//     detaching. Ensures threads are not left joinable on destruction. Not copyable.
//     Errors are threated as release mode asserts instead of being tracked.

// hxmutex - std::mutex style wrapper for pthreads. Asserts on unexpected failure
// by the posix api. Currently default pthread behavior. That means non-recursive,
// no error-checking and no translation layer.
class hxmutex {
public:
    // Constructs a mutex and initializes it. May not return if the mutex cant
    // be initialzed correctly. Something is very wrong if this fails.
     inline hxmutex() {
        int code_ = ::pthread_mutex_init(&m_mutex_, 0);
        hxassertrelease(code_ == 0, "pthread_mutex_init %s", ::strerror(code_));
        (void)code_;
    }

    // Destroys the mutex.
    inline ~hxmutex() {
        int code_ = ::pthread_mutex_destroy(&m_mutex_);
        hxassertmsg(code_ == 0, "pthread_mutex_destroy %s", ::strerror(code_));
        (void)code_;
    }

    // Locks the mutex. Returns true on success, asserts and returns false
    // otherwise. Something is very wrong if this fails.
    inline bool lock() {
        int code_ = ::pthread_mutex_lock(&m_mutex_);
        hxassertmsg(code_ == 0, "pthread_mutex_lock %s", ::strerror(code_));
        return code_ == 0;
    }

    // Unlocks the mutex. Returns true on success, asserts and returns false
    // otherwise. It is undefined if you unlock a mutex that you have not locked
    // and such an operation may succeed.
    inline bool unlock() {
        int code_ = ::pthread_mutex_unlock(&m_mutex_);
        hxassertmsg(code_ == 0, "pthread_mutex_unlock %s", ::strerror(code_));
        return code_ == 0;
    }

    // Returns a pointer to the native pthread mutex handle.
    inline pthread_mutex_t* native_handle() { return &m_mutex_; }

private:
    // Deleted copy constructor.
    hxmutex(const hxmutex&) hxdelete_fn;
    // Deleted copy assignment operator.
    hxmutex& operator=(const hxmutex&) hxdelete_fn;

	pthread_mutex_t m_mutex_;
};

// hxunique_lock - std::unique_lock style RAII-style unique lock for hxmutex.
// Locks the mutex on construction and unlocks on destruction.
class hxunique_lock {
public:
    // Constructs with option to defer locking.
    // - defer_lock: If true, does not lock the mutex immediately.
    inline hxunique_lock(hxmutex& m_, bool defer_lock_=false)
            : m_mutex_(m_), m_owns_(false) {
        if (!defer_lock_) {
            lock();
        }
    }
    // Unlocks the mutex if owned.
    inline ~hxunique_lock() {
        if (m_owns_) {
            unlock();
        }
    }
    // Locks the mutex if not already locked.
    inline void lock() {
        if (!m_owns_) {
            m_owns_ = m_mutex_.lock();
        }
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
    inline hxcondition_variable() {
        int code_ = ::pthread_cond_init(&m_cond_, 0);
        hxassertrelease(code_ == 0, "pthread_cond_init %s", ::strerror(code_));
        (void)code_;
    }

    // Destroys the condition variable if valid.
    inline ~hxcondition_variable() {
        int code_ = ::pthread_cond_destroy(&m_cond_);
        hxassertmsg(code_ == 0, "pthread_cond_destroy %s", ::strerror(code_));
        (void)code_;
    }

    // Waits for the condition variable to be notified. Returns true on success,
    // false otherwise.
    // - mutex: The mutex to use for waiting.
    inline bool wait(hxmutex& mutex_) {
        int code_ = ::pthread_cond_wait(&m_cond_, mutex_.native_handle());
        hxassertmsg(code_ == 0, "pthread_cond_init %s", ::strerror(code_));
        return code_ == 0;
    }

    // Overload: Waits using a hxunique_lock. Returns true on success, false
    // otherwise.
    // - lock: The unique lock to use for waiting.
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

    // Notifies one waiting thread. Returns true on success, false otherwise.
    inline bool notify_one() {
        int code_ = ::pthread_cond_signal(&m_cond_);
        hxassertmsg(code_ == 0, "pthread_cond_init %s", ::strerror(code_));
        return code_ == 0;
    }

    // Notifies all waiting threads. Returns true on success, false otherwise.
    inline bool notify_all() {
        int code_ = ::pthread_cond_broadcast(&m_cond_);
        hxassertmsg(code_ == 0, "pthread_cond_init %s", ::strerror(code_));
        return code_ == 0;
    }

    // Returns a pointer to the native pthread condition variable handle.
    inline pthread_cond_t* native_handle() { return &m_cond_; }

private:
    // Deleted copy constructor.
    hxcondition_variable(const hxcondition_variable&) hxdelete_fn;
    // Deleted copy assignment operator.
    hxcondition_variable& operator=(const hxcondition_variable&) hxdelete_fn;

	pthread_cond_t m_cond_;
};

// hxthread - std::thread style thread wrapper for pthreads. Provides thread
// creation and joining.
class hxthread {
public:
    // Default constructor. Thread is not started.
    inline hxthread() : m_started_(false), m_joined_(false) { }

    // Constructs and starts a thread with the given function and argument.
    // Does not free arg. Any function that takes a single pointer and returns
    // a void pointer should work. The return value is ignored but may be unsafe
    // to cast to a function with a different return type.
    // - entry_point: Function pointer of type: void* fn(T*).
    // - parameter: T* to pass to the function.
    template<typename parameter_t_>
    inline explicit hxthread(void* (*entry_point_)(parameter_t_*), parameter_t_* parameter_)
            : m_started_(false), m_joined_(false) {
        this->start(entry_point_, parameter_);
    }

    // Destructor. Asserts that the thread was stopped correctly.
    inline ~hxthread() {
		hxassertmsg(!this->joinable(), "thread still running");
    }

    // Starts a thread with the given function and argument. Does not free arg.
    // Any function that takes a single T pointer and returns a void pointer
    // should work. The return value is ignored but is required by pthreads
    // calling convention.
    // - entry_point: Function pointer of type: void* entry_point(T*).
    // - parameter: T* to pass to the function.
    template<typename parameter_t_>
    inline void start(void* (*entry_point_)(parameter_t_*), parameter_t_* parameter_) {
		hxassertmsg(!this->joinable(), "thread still running");

        // Stay on the right side of the C++ standard by avoiding assumptions
        // about pointer representations. The parameter_ pointer is never cast
        // between types. Instead the bit pattern of the pointer is preserved
        // while it is passed through the pthread api. This requires the pointers
        // to use the same number of bytes.
        hxstatic_assert(sizeof(void*) == sizeof(parameter_t_*), "incompatible pointer types");

        void* reinterpreted_parameter_=hxnull;
        ::memcpy(&reinterpreted_parameter_, &parameter_, sizeof(void*));
        int code_ = ::pthread_create(&m_thread_, 0, (entry_point_function_t_)entry_point_,
            reinterpreted_parameter_);

        hxassertrelease(code_ == 0, "pthread_create %d", code_); (void)code_;
        m_started_ = true;
        m_joined_ = false;
    }

    // Returns true if the thread has been started and not yet joined.
    inline bool joinable() const { return m_started_ && !m_joined_; }

    // Joins the thread. Blocks until the thread finishes.
    inline void join() {
        hxassertmsg(this->joinable(), "thread not runnning");
        int code_ = ::pthread_join(m_thread_, 0);
        hxassertrelease(code_ == 0, "pthread_join %s", ::strerror(code_));
        m_joined_ = true;
    }

private:
    // Type expected by pthread.
    typedef void* (*entry_point_function_t_)(void*);

    // Deleted copy constructor.
    hxthread(const hxthread&) hxdelete_fn;
    // Deleted copy assignment operator.
    hxthread& operator=(const hxthread&) hxdelete_fn;

    pthread_t m_thread_;
    bool m_started_;
    bool m_joined_;
};

#endif //HX_USE_THREADS
