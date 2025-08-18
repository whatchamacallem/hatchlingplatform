#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.
//
// <hx/hxthread.hpp> - Threading primitives that mostly adhere to the C++
// standard. This header provides lightweight C++ wrappers around POSIX pthreads
// for thread synchronization and management. The following classes are defined:
//
// - hxthread_local<T> (Available single threaded as well.)
//	 Provides a C++ template for thread-local storage, allowing each thread to
//	 maintain its own instance of a specified type T. This class is available
//	 for compatibility when threading is off.
//
// - hxmutex (HX_USE_THREADS only)
//	 Mutex wrapper for pthreads. Provides lock/unlock functionality, error
//	 tracking, and ensures proper initialization and destruction. Not copyable.
//	 Asserts on incorrect configuration.
//
// - hxunique_lock (HX_USE_THREADS only)
//	 RAII-style unique lock for hxmutex. Locks the mutex on construction and
//	 unlocks on destruction. Supports deferred locking and ownership checks.
//	 Not copyable.
//
// - hxcondition_variable (HX_USE_THREADS only)
//	 Condition variable wrapper for pthreads. Allows threads to wait for
//	 notifications, supports predicate-based waiting, and provides notify_one
//	 and notify_all methods. Not copyable. Asserts on errors.
//
// - hxthread (HX_USE_THREADS only)
//	 Thread wrapper for pthreads. Provides thread creation, joining, and
//	 detaching. Ensures threads are not left joinable on destruction. Not copyable.
//	 Errors are threated as release mode asserts instead of being tracked.

#include <hx/hatchling.h>

#if HX_USE_THREADS
#include <errno.h>
#include <pthread.h>
#endif

/// Return the current thread id. Returns `0` when threads are disabled.
inline size_t hxthread_id() {
#if HX_USE_THREADS
	return (size_t)pthread_self();
#else
	return 0; // Single threaded.
#endif
}

/// `hxthread_local<T>` - Provides a C++ template for thread-local storage, allowing
/// each thread to maintain its own instance of a specified type T. This class is
/// available for compatibility when threading is off.
template<typename T_>
class hxthread_local {
public:
	/// Construct with default value for each thread.
	explicit hxthread_local(const T_& default_value_ = T_())
			: m_default_value_(default_value_) {
#if HX_USE_THREADS
		int code_ = pthread_key_create(&m_key_, destroy_local_);
		hxassertrelease(code_ == 0, "pthread_key_create %s", ::strerror(code_)); (void)code_;
#endif
	}

	/// Destroy every thread's private copy.
	~hxthread_local() {
#if HX_USE_THREADS
		pthread_key_delete(m_key_);
#endif
	}

	/// Set the thread local value from `T`.
	void operator=(const T_& local_) { *get_local_() = local_; }

	/// Cast the thread local value to `T`.
	operator const T_&() const { return *get_local_(); }
	operator T_&() { return *get_local_(); }

	/// "address of" operator returns `T*`.
	const T_* operator&() const { return get_local_(); }
	T_* operator&() { return get_local_(); }

private:
	// This is a form of "mutable when const." A thread should not
	// know or care when storage is allocated for it.
#if HX_USE_THREADS
	T_* get_local_() const {
		T_* local_ = static_cast<T_*>(pthread_getspecific(m_key_));
		if (!local_) {
			local_ = new T_(m_default_value_);
			hxassertrelease(local_, "new T");
			int code_ = pthread_setspecific(m_key_, local_);
			hxassertrelease(code_ == 0, "pthread_setspecific %s", ::strerror(code_)); (void)code_;
		}
		return local_;
	}
#else
	const T_* get_local_() const { return &m_default_value_; }
	T_* get_local_() { return &m_default_value_; }
#endif

	static void destroy_local_(void* ptr_) noexcept {
		if (ptr_) {
			delete static_cast<T_*>(ptr_);
		}
	}

	explicit hxthread_local(const hxthread_local&) = delete;
	hxthread_local& operator=(const hxthread_local&) = delete;

#if HX_USE_THREADS
	pthread_key_t m_key_;
#endif
	T_ m_default_value_;
};

// The remaining classes are only available when threading is enabled. Emulating
// pthreads is a little too nutty because it has a range of valid implementations.
#if HX_USE_THREADS

/// `hxmutex` - `std::mutex` style wrapper for pthreads. Asserts on unexpected failure
/// by the posix api. Currently default pthread behavior. That means non-recursive,
/// no error-checking and no translation layer.
class hxmutex {
public:
	/// Constructs a mutex and initializes it. May not return if the mutex cant
	/// be initialzed correctly. Something is very wrong if this fails.
	 inline hxmutex(void) {
		int code_ = ::pthread_mutex_init(&m_mutex_, 0);
		hxassertrelease(code_ == 0, "pthread_mutex_init %s", ::strerror(code_)); (void)code_;
	}

	/// Destroys the mutex.
	~hxmutex(void) {
		int code_ = ::pthread_mutex_destroy(&m_mutex_);
		hxassertmsg(code_ == 0, "pthread_mutex_destroy %s", ::strerror(code_)); (void)code_;
	}

	/// Locks the mutex. Returns true on success, asserts on invalid arguments
	/// and returns false on failure.
	bool lock(void) {
		int code_ = ::pthread_mutex_lock(&m_mutex_);
		hxassertmsg(code_ == 0 || code_ == EBUSY || code_ == EAGAIN, "pthread_mutex_lock %s", ::strerror(code_));
		return code_ == 0;
	}

	/// Unlocks the mutex. Returns true on success, asserts and returns false
	/// otherwise. It is undefined if you unlock a mutex that you have not locked
	/// and such an operation may succeed.
	bool unlock(void) {
		int code_ = ::pthread_mutex_unlock(&m_mutex_);
		hxassertmsg(code_ == 0, "pthread_mutex_unlock %s", ::strerror(code_));
		return code_ == 0;
	}

	/// Returns a pointer to the native pthread mutex handle.
	pthread_mutex_t* native_handle(void) { return &m_mutex_; }

private:
	// Deleted copy constructor.
	hxmutex(const hxmutex&) = delete;
	// Deleted copy assignment operator.
	hxmutex& operator=(const hxmutex&) = delete;

	pthread_mutex_t m_mutex_;
};

/// `hxunique_lock` - `std::unique_lock` style RAII-style unique lock for `hxmutex`.
/// Locks the mutex on construction and unlocks on destruction.
class hxunique_lock {
public:
	/// Constructs with option to defer locking.
	/// - `defer_lock` : If true, does not lock the mutex immediately.
	hxunique_lock(hxmutex& mtx_, bool defer_lock_=false)
			: m_mutex_(mtx_), m_owns_(false) {
		if (!defer_lock_) {
			lock();
		}
	}
	/// Unlocks the mutex if owned.
	~hxunique_lock(void) {
		if (m_owns_) {
			unlock();
		}
	}
	/// Locks the mutex if not already locked.
	void lock(void) {
		if (!m_owns_) {
			m_owns_ = m_mutex_.lock();
		}
	}
	/// Unlocks the mutex if owned.
	void unlock(void) {
		if (m_owns_) {
			m_mutex_.unlock();
			m_owns_ = false;
		}
	}

	/// Returns true if the lock owns the mutex.
	bool owns_lock(void) const { return m_owns_; }

	/// Returns a reference to the associated mutex.
	hxmutex& mutex(void) { return m_mutex_; }

private:
	// Deleted copy constructor.
	hxunique_lock(const hxunique_lock&) = delete;
	// Deleted copy assignment operator.
	hxunique_lock& operator=(const hxunique_lock&) = delete;
	hxmutex& m_mutex_;
	bool m_owns_;
};

/// `hxcondition_variable` - `std::condition_variable` style condition variable
/// wrapper for pthreads. Allows threads to wait for notifications.
class hxcondition_variable {
public:
	/// Constructs and initializes the condition variable.
	hxcondition_variable(void) {
		int code_ = ::pthread_cond_init(&m_cond_, 0);
		hxassertrelease(code_ == 0, "pthread_cond_init %s", ::strerror(code_)); (void)code_;
	}

	/// Destroys the condition variable if valid.
	~hxcondition_variable(void) {
		int code_ = ::pthread_cond_destroy(&m_cond_);
		hxassertmsg(code_ == 0, "pthread_cond_destroy %s", ::strerror(code_)); (void)code_;
	}

	/// Waits for the condition variable to be notified. Returns true on success,
	/// false otherwise.
	/// - `mutex` : The mutex to use for waiting.
	bool wait(hxmutex& mutex_) {
		int code_ = ::pthread_cond_wait(&m_cond_, mutex_.native_handle());
		hxassertmsg(code_ == 0, "pthread_cond_init %s", ::strerror(code_));
		return code_ == 0;
	}

	/// Overload: Waits using a `hxunique_lock`. Returns true on success, false
	/// otherwise.
	/// - `lock` : The unique lock to use for waiting.
	bool wait(hxunique_lock& lock_) {
		return wait(lock_.mutex());
	}

	/// Waits until the predicate returns true.
	/// - `lock` : The unique lock to use for waiting.
	/// - `pred` : Predicate function to check.
	template<typename predicate_t_>
	void wait(hxunique_lock& lock_, predicate_t_ pred_) {
		while (!pred_()) {
			wait(lock_);
		}
	}

	/// Notifies one waiting thread. Returns true on success, false otherwise.
	bool notify_one(void) {
		int code_ = ::pthread_cond_signal(&m_cond_);
		hxassertmsg(code_ == 0, "pthread_cond_init %s", ::strerror(code_));
		return code_ == 0;
	}

	/// Notifies all waiting threads. Returns true on success, false otherwise.
	bool notify_all(void) {
		int code_ = ::pthread_cond_broadcast(&m_cond_);
		hxassertmsg(code_ == 0, "pthread_cond_init %s", ::strerror(code_));
		return code_ == 0;
	}

	/// Returns a pointer to the native pthread condition variable handle.
	pthread_cond_t* native_handle(void) { return &m_cond_; }

private:
	// Deleted copy constructor.
	hxcondition_variable(const hxcondition_variable&) = delete;
	// Deleted copy assignment operator.
	hxcondition_variable& operator=(const hxcondition_variable&) = delete;

	pthread_cond_t m_cond_;
};

/// `hxthread` - `std::thread` style thread wrapper for pthreads. Provides thread
/// creation and joining.
class hxthread {
public:
	/// Default constructor. Thread is not started.
	hxthread() : m_started_(false), m_joined_(false) { }

	/// Constructs and starts a thread with the given function and argument.
	/// Does not free arg. Any function that takes a single pointer and returns
	/// a void pointer should work. The return value is ignored but may be unsafe
	/// to cast to a function with a different return type.
	/// - `entry_point` : Function pointer of type: void* fn(T*).
	/// - `parameter` : T* to pass to the function.
	template<typename parameter_t_>
	explicit hxthread(void* (*entry_point_)(parameter_t_*), parameter_t_* parameter_)
			: m_started_(false), m_joined_(false) {
		this->start(entry_point_, parameter_);
	}

	/// Destructor. Asserts that the thread was stopped correctly.
	~hxthread(void) {
		hxassertmsg(!this->joinable(), "thread_still_running");
	}

	/// Starts a thread with the given function and argument. Does not free arg.
	/// Any function that takes a single `T` pointer and returns a `void` pointer
	/// should work. The return value is ignored but is required by pthreads
	/// calling convention.
	/// - `entry_point` : Function pointer of type: void* entry_point(T*).
	/// - `parameter` : T* to pass to the function.
	template<typename parameter_t_>
	void start(void* (*entry_point_)(parameter_t_*), parameter_t_* parameter_) {
		hxassertmsg(!this->joinable(), "thread_still_running");

		// Stay on the right side of the C++ standard by avoiding assumptions
		// about pointer representations. The parameter pointer is never cast
		// between types. Instead the bit pattern of the pointer is preserved
		// while it is passed through the pthread api. This requires the pointers
		// to use the same number of bytes.
		static_assert(sizeof(void*) == sizeof(parameter_t_*), "Incompatible pointer types");

		void* reinterpreted_parameter_=hxnull;
		::memcpy(&reinterpreted_parameter_, &parameter_, sizeof(void*));
		int code_ = ::pthread_create(&m_thread_, 0, (entry_point_function_t_)entry_point_,
			reinterpreted_parameter_);

		hxassertrelease(code_ == 0, "pthread_create %d", code_); (void)code_;
		m_started_ = true;
		m_joined_ = false;
	}

	/// Returns true if the thread has been started and not yet joined.
	bool joinable(void) const { return m_started_ && !m_joined_; }

	/// Joins the thread. Blocks until the thread finishes.
	void join(void) {
		hxassertmsg(this->joinable(), "thread_not_runnning");
		int code_ = ::pthread_join(m_thread_, 0);
		hxassertrelease(code_ == 0, "pthread_join %s", ::strerror(code_));
		(void)code_;
		m_joined_ = true;
	}

private:
	// Type expected by pthread.
	typedef void* (*entry_point_function_t_)(void*);

	// Deleted copy constructor.
	hxthread(const hxthread&) = delete;

	// Deleted copy assignment operator.
	hxthread& operator=(const hxthread&) = delete;

	pthread_t m_thread_;
	bool m_started_;
	bool m_joined_;
};

#endif //HX_USE_THREADS
