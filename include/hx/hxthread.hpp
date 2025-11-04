#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

/// \file hx/hxthread.hpp Threading primitives that mostly adhere to the C++
/// standard. This header provides lightweight C++ wrappers around POSIX
/// pthreads or C11 `threads.h` primitives for thread synchronization and
/// management. The following classes are defined:
///
/// - `hxthread_local<T>` (Available single-threaded as well.) Provides a C++
///	  template for thread-local storage, allowing each thread to maintain its own
///	  instance of a specified type `T`. This class is available for compatibility
///	  when threading is off.
///
/// - `hxmutex` (`HX_USE_THREADS` only) Mutex wrapper for the configured thread
///	  backend. Provides
///	  lock/unlock functionality, error tracking, and ensures proper initialization
///	  and destruction. Not copyable. Asserts on incorrect configuration.
///
/// - `hxunique_lock` (`HX_USE_THREADS` only) RAII-style unique lock for
///	  `hxmutex`. Locks the mutex on construction and unlocks on destruction.
///	  Supports deferred locking and ownership checks. Not copyable.
///
/// - `hxcondition_variable` (`HX_USE_THREADS` only) Condition variable wrapper
///	  for the native backend. Allows threads to wait for notifications, supports
///	  predicate-based waiting, and provides `notify_one` and `notify_all` methods.
///	  Not copyable. Asserts on errors.
///
/// - `hxthread` (`HX_USE_THREADS` only) Thread wrapper for the native backend.
///	  thread creation, joining, and detaching. Ensures threads are not left
///	  joinable on destruction. Not copyable. Errors are treated as release-mode
///	  asserts instead of being tracked.
///
/// For atomics consider `<stdatomic.h>`. It is the same as `std::atomic`.

#include "hatchling.h"
#include "hxutility.h"

#if HX_USE_THREADS
#include <errno.h>

#if defined(__has_include) && __has_include(<threads.h>)
#define HX_USE_C11_THREADS 1
#include <threads.h>
#else
#define HX_USE_C11_THREADS 0
#include <pthread.h>
#endif
#endif

/// `hxthread_local<T>` - Provides a C++ template for thread-local storage, allowing
/// each thread to maintain its own instance of a specified type T. This class is
/// available for compatibility when threading is off.
template<typename T_>
class hxthread_local {
public:
	/// Constructs with a default value for each thread.
	explicit hxthread_local(const T_& default_value_ = T_())
			: m_default_value_(default_value_) {
#if HX_USE_THREADS
#if HX_USE_C11_THREADS
		const int code_ = ::tss_create(&m_key_, destroy_local_);
		hxassertrelease(code_ == thrd_success, "tss_create %d", code_); (void)code_;
#else
		const int code_ = ::pthread_key_create(&m_key_, destroy_local_);
		hxassertrelease(code_ == 0, "pthread_key_create %s", ::strerror(code_)); (void)code_;
#endif
#endif
	}

	/// Frees resources.
	~hxthread_local() {
#if HX_USE_THREADS
#if HX_USE_C11_THREADS
		::tss_delete(m_key_);
#else
		::pthread_key_delete(m_key_);
#endif
#endif
	}

	/// Sets the thread-local value from `T`.
	template<class U_>
	void operator=(U_&& local_) { *(this->get_local_()) = hxforward<U_>(local_); }

	/// Casts the thread-local value to `T&`.
	operator const T_&() const { return *(this->get_local_()); }
	operator T_&() { return *(this->get_local_()); }

	/// The "address of" operator returns `T*`.
	const T_* operator&() const { return this->get_local_(); }
	T_* operator&() { return this->get_local_(); }

private:
	// This is a form of "mutable when const." A thread should not
	// know or care when storage is allocated for it.
#if HX_USE_THREADS
	T_* get_local_() const {
#if HX_USE_C11_THREADS
		T_* local_ = static_cast<T_*>(::tss_get(m_key_));
		if(local_ == hxnull) {
			local_ = new T_(m_default_value_);
			hxassertrelease(local_, "new T");
			const int code_ = ::tss_set(m_key_, local_);
			hxassertrelease(code_ == thrd_success, "tss_set %d", code_); (void)code_;
		}
		return local_;
#else
		T_* local_ = static_cast<T_*>(::pthread_getspecific(m_key_));
		if(local_ == hxnull) {
			local_ = new T_(m_default_value_);
			hxassertrelease(local_, "new T");
			const int code_ = ::pthread_setspecific(m_key_, local_);
			hxassertrelease(code_ == 0, "pthread_setspecific %s", ::strerror(code_)); (void)code_;
		}
		return local_;
#endif
	}
#else
	const T_* get_local_() const { return &m_default_value_; }
	T_* get_local_() { return &m_default_value_; }
#endif

	static void destroy_local_(void* ptr_) noexcept {
		hxassertmsg(ptr_, "destroy_local_");
		delete static_cast<T_*>(ptr_);
	}

	explicit hxthread_local(const hxthread_local&) = delete;
	hxthread_local& operator=(const hxthread_local&) = delete;

#if HX_USE_THREADS
#if HX_USE_C11_THREADS
	::tss_t m_key_;
#else
	::pthread_key_t m_key_;
#endif
#endif
	T_ m_default_value_;
};

/// Returns the current thread ID. Returns `0` when threads are disabled. This
/// is used by the profiler and so it tries to be efficient.
inline size_t hxthread_id() {
#if HX_USE_THREADS
#if HX_USE_C11_THREADS
    static hxthread_local<size_t> tid_;
    return reinterpret_cast<uintptr_t>(&tid_);
#else
	return static_cast<size_t>(::pthread_self());
#endif
#else
	return 0; // Single threaded.
#endif
}

// The remaining classes are only available when threading is enabled. Emulating
// pthreads is a little too nutty because it has a range of valid implementations.
#if HX_USE_THREADS

/// `hxmutex` - `std::mutex` style wrapper for the configured thread backend.
/// Asserts on unexpected failure by the native API. Currently default behavior
/// mirrors non-recursive, no error-checking primitives. That means
/// non-recursive, no error-checking and no translation layer.
class hxmutex {
public:
	/// Constructs a mutex and initializes it. May not return if the mutex can't
	/// be initialized correctly. Something is very wrong if this fails.
	inline hxmutex(void) {
#if HX_USE_C11_THREADS
		const int code_ = ::mtx_init(&m_mutex_, mtx_plain);
		hxassertrelease(code_ == thrd_success, "mtx_init %d", code_); (void)code_;
#else
		const int code_ = ::pthread_mutex_init(&m_mutex_, 0);
		hxassertrelease(code_ == 0, "pthread_mutex_init %s", ::strerror(code_)); (void)code_;
#endif
	}

	/// Destroys the mutex.
	~hxmutex(void) {
#if HX_USE_C11_THREADS
		::mtx_destroy(&m_mutex_);
#else
		const int code_ = ::pthread_mutex_destroy(&m_mutex_);
		hxassertmsg(code_ == 0, "pthread_mutex_destroy %s", ::strerror(code_)); (void)code_;
#endif
	}

	/// Locks the mutex. Returns true on success, asserts on invalid arguments,
	/// and returns false on failure. Callers must check the return value and
	/// avoid ignoring lock failures.
	bool lock(void) hxattr_nodiscard {
#if HX_USE_C11_THREADS
		const int code_ = ::mtx_lock(&m_mutex_);
		hxassertmsg(code_ == thrd_success, "mtx_lock %d", code_);
		return code_ == thrd_success;
#else
		const int code_ = ::pthread_mutex_lock(&m_mutex_);
		hxassertmsg(code_ == 0 || code_ == EBUSY || code_ == EAGAIN, "pthread_mutex_lock %s", ::strerror(code_));
		return code_ == 0;
#endif
	}

	/// Unlocks the mutex. Returns true on success; asserts and returns false
	/// otherwise. It is undefined to unlock a mutex that you have not locked, and
	/// such an operation may succeed.
	bool unlock(void) {
#if HX_USE_C11_THREADS
		const int code_ = ::mtx_unlock(&m_mutex_);
		hxassertmsg(code_ == thrd_success, "mtx_unlock %d", code_);
		return code_ == thrd_success;
#else
		const int code_ = ::pthread_mutex_unlock(&m_mutex_);
		hxassertmsg(code_ == 0, "pthread_mutex_unlock %s", ::strerror(code_));
		return code_ == 0;
#endif
	}

	/// Returns a pointer to the native mutex handle.
#if HX_USE_C11_THREADS
	::mtx_t* native_handle(void) { return &m_mutex_; }
#else
	::pthread_mutex_t* native_handle(void) { return &m_mutex_; }
#endif

private:
	// Deleted copy constructor.
	hxmutex(const hxmutex&) = delete;
	// Deleted copy assignment operator.
	hxmutex& operator=(const hxmutex&) = delete;

#if HX_USE_C11_THREADS
	::mtx_t m_mutex_;
#else
	::pthread_mutex_t m_mutex_;
#endif
};

/// `hxunique_lock` - `std::unique_lock` style RAII-style unique lock for `hxmutex`.
/// Locks the mutex on construction and unlocks on destruction.
class hxunique_lock {
public:
	/// Constructs with an option to defer locking.
	/// - `defer_lock` : If true, does not lock the mutex immediately.
	hxunique_lock(hxmutex& mtx_, bool defer_lock_=false)
			: m_mutex_(mtx_), m_owns_(false) {
		if(!defer_lock_) {
			this->lock();
		}
	}
	/// Unlocks the mutex if owned.
	~hxunique_lock(void) {
		if(m_owns_) {
			this->unlock();
		}
	}
	/// Locks the mutex if not already locked.
	void lock(void) {
		if(!m_owns_) {
			m_owns_ = m_mutex_.lock();
		}
	}
	/// Unlocks the mutex if owned.
	void unlock(void) {
		if(m_owns_) {
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
/// wrapper for the configured thread backend. Allows threads to wait for
/// notifications.
class hxcondition_variable {
public:
	/// Constructs and initializes the condition variable.
	hxcondition_variable(void) {
#if HX_USE_C11_THREADS
		const int code_ = ::cnd_init(&m_cond_);
		hxassertrelease(code_ == thrd_success, "cnd_init %d", code_); (void)code_;
#else
		const int code_ = ::pthread_cond_init(&m_cond_, 0);
		hxassertrelease(code_ == 0, "pthread_cond_init %s", ::strerror(code_)); (void)code_;
#endif
	}

	/// Destroys the condition variable if valid.
	~hxcondition_variable(void) {
#if HX_USE_C11_THREADS
		::cnd_destroy(&m_cond_);
#else
		const int code_ = ::pthread_cond_destroy(&m_cond_);
		hxassertmsg(code_ == 0, "pthread_cond_destroy %s", ::strerror(code_)); (void)code_;
#endif
	}

	/// Waits for the condition variable to be notified. Returns true on success,
	/// false otherwise. Callers must check the return value to confirm the wait
	/// succeeded.
	/// - `mutex` : The mutex to use for waiting.
	bool wait(hxmutex& mutex_) hxattr_nodiscard {
#if HX_USE_C11_THREADS
		const int code_ = ::cnd_wait(&m_cond_, mutex_.native_handle());
		hxassertmsg(code_ == thrd_success, "cnd_wait %d", code_);
		return code_ == thrd_success;
#else
		const int code_ = ::pthread_cond_wait(&m_cond_, mutex_.native_handle());
		hxassertmsg(code_ == 0, "pthread_cond_init %s", ::strerror(code_));
		return code_ == 0;
#endif
	}

	/// Overload: Waits using a `hxunique_lock`. Returns true on success, false
	/// otherwise. Callers must check the return value to confirm the wait
	/// succeeded.
	/// - `lock` : The unique lock to use for waiting.
	bool wait(hxunique_lock& lock_) hxattr_nodiscard {
		return this->wait(lock_.mutex());
	}

	/// Waits until the predicate returns true.
	/// - `lock` : The unique lock to use for waiting.
	/// - `pred` : Predicate function to check.
	template<typename predicate_t_>
	void wait(hxunique_lock& lock_, predicate_t_ pred_) {
		while(!pred_()) {
			// Failure is undefined as per the standard.
			const bool wait_result = this->wait(lock_);
			hxassertmsg(wait_result, "wait"); (void)wait_result;
		}
	}

	/// Notifies one waiting thread. Returns true on success, false otherwise.
	bool notify_one(void) {
#if HX_USE_C11_THREADS
		const int code_ = ::cnd_signal(&m_cond_);
		hxassertmsg(code_ == thrd_success, "cnd_signal %d", code_);
		return code_ == thrd_success;
#else
		const int code_ = ::pthread_cond_signal(&m_cond_);
		hxassertmsg(code_ == 0, "pthread_cond_init %s", ::strerror(code_));
		return code_ == 0;
#endif
	}

	/// Notifies all waiting threads. Returns true on success, false otherwise.
	bool notify_all(void) {
#if HX_USE_C11_THREADS
		const int code_ = ::cnd_broadcast(&m_cond_);
		hxassertmsg(code_ == thrd_success, "cnd_broadcast %d", code_);
		return code_ == thrd_success;
#else
		const int code_ = ::pthread_cond_broadcast(&m_cond_);
		hxassertmsg(code_ == 0, "pthread_cond_init %s", ::strerror(code_));
		return code_ == 0;
#endif
	}

	/// Returns a pointer to the native condition variable handle.
#if HX_USE_C11_THREADS
	::cnd_t* native_handle(void) { return &m_cond_; }
#else
	::pthread_cond_t* native_handle(void) { return &m_cond_; }
#endif

private:
	// Deleted copy constructor.
	hxcondition_variable(const hxcondition_variable&) = delete;
	// Deleted copy assignment operator.
	hxcondition_variable& operator=(const hxcondition_variable&) = delete;

#if HX_USE_C11_THREADS
	::cnd_t m_cond_;
#else
	::pthread_cond_t m_cond_;
#endif
};

/// `hxthread` - `std::thread` style thread wrapper for the configured backend.
/// Provides thread creation and joining.
class hxthread {
public:
	/// Default constructor. Thread is not started.
	hxthread() : m_thread_(), m_started_(false), m_joined_(false) { }

	/// Constructs and starts a thread with the given function and argument. Does
	/// not free the argument. Any function that takes a single pointer and
	/// returns a void pointer should work. The return value is ignored but may be
	/// unsafe to cast to a function with a different return type.
	/// - `entry_point` : Function pointer of type: void* fn(T*).
	/// - `parameter` : T* to pass to the function.
	template<typename parameter_t_>
	explicit hxthread(void* (*entry_point_)(parameter_t_*), parameter_t_* parameter_)
			: m_thread_(), m_started_(false), m_joined_(false) {
		this->start(entry_point_, parameter_);
	}

	/// Destructor. Asserts that the thread was stopped correctly.
	~hxthread(void) {
		hxassertmsg(!this->joinable(), "thread_still_running");
	}

	/// Starts a thread with the given function and argument. Does not free the
	/// argument. Any function that takes a single `T` pointer and returns a
	/// `void` pointer should work. The return value is ignored but is required by
	/// the native calling convention.
	/// - `entry_point` : Function pointer of type: void* entry_point(T*).
	/// - `parameter` : T* to pass to the function.
	template<typename parameter_t_>
	void start(void* (*entry_point_)(parameter_t_*), parameter_t_* parameter_) {
		hxassertmsg(!this->joinable(), "thread_still_running");

		// Initialize this single threaded as local statics may not be locked.
 		hxthread_id();

		// Stay on the right side of the C++ standard by avoiding assumptions
		// about pointer representations. The parameter is being reinterpreted
		// twice instead of cast once and reinterpreted back.
		static_assert(sizeof(void*) == sizeof(parameter_t_*), "Incompatible pointer sizes.");

		void* reinterpreted_parameter_ = hxnull;
		::memcpy(&reinterpreted_parameter_, &parameter_, sizeof(void*)); // NOLINT
#if HX_USE_C11_THREADS
		entry_point_function_t_ native_entry_ = reinterpret_cast<entry_point_function_t_>(entry_point_);
		const int code_ = ::thrd_create(&m_thread_, native_entry_, reinterpreted_parameter_);
		hxassertrelease(code_ == thrd_success, "thrd_create %d", code_); (void)code_;
#else
		const int code_ = ::pthread_create(&m_thread_, 0, reinterpret_cast<entry_point_function_t_>(entry_point_),
			reinterpreted_parameter_);

		hxassertrelease(code_ == 0, "pthread_create %d", code_); (void)code_;
#endif
		m_started_ = true;
		m_joined_ = false;
	}

	/// Returns true if the thread has been started and not yet joined. Callers
	/// must check the return value before acting on the thread state.
	bool joinable(void) const hxattr_nodiscard { return m_started_ && !m_joined_; }

	/// Joins the thread. Blocks until the thread finishes.
	void join(void) {
		hxassertmsg(this->joinable(), "thread_not_runnning");
#if HX_USE_C11_THREADS
		const int code_ = ::thrd_join(m_thread_, hxnull);
		hxassertrelease(code_ == thrd_success, "thrd_join %d", code_);
		(void)code_;
#else
		const int code_ = ::pthread_join(m_thread_, 0);
		hxassertrelease(code_ == 0, "pthread_join %s", ::strerror(code_));
		(void)code_;
#endif
		m_joined_ = true;
	}

private:
#if HX_USE_C11_THREADS
	typedef int (*entry_point_function_t_)(void*);
#else
	typedef void* (*entry_point_function_t_)(void*);
#endif

	// Deleted copy constructor.
	hxthread(const hxthread&) = delete;

	// Deleted copy assignment operator.
	hxthread& operator=(const hxthread&) = delete;

#if HX_USE_C11_THREADS
	::thrd_t m_thread_;
#else
	::pthread_t m_thread_;
#endif
	bool m_started_;
	bool m_joined_;
};

#endif // HX_USE_THREADS
