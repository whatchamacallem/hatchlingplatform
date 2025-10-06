// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include <hx/hxthread.hpp>
#include <hx/hxtest.hpp>

#if HX_USE_THREADS

namespace {

struct hxthread_test_simple_parameters_t {
	hxmutex* mutex;
	int* shared;
};

class hxthread_test_parameters_t {
public:
	hxmutex* mutex;
	hxcondition_variable* condition_variable;
	bool* ready;
	int* woken;

	hxthread_test_parameters_t(hxmutex* mutex_, hxcondition_variable* condition_variable_,
								bool* ready_, int* woken_)
		: mutex(mutex_), condition_variable(condition_variable_), ready(ready_), woken(woken_) { }
};

class hxthread_test_predicate_wait_for_zero {
public:
	explicit hxthread_test_predicate_wait_for_zero(int* v) : value(v) {}
	bool operator()(void) const { return *value == 0; }
	int* value;
};

void* hxthread_test_func_increment(hxthread_test_simple_parameters_t* parameters) {
	hxunique_lock lock(*parameters->mutex);
	++(*parameters->shared);
	return hxnull;
}

void* hxthread_test_func_notify_one(hxthread_test_parameters_t* parameters) {
	hxunique_lock lock(*parameters->mutex);
	while(!*parameters->ready) {
		parameters->condition_variable->wait(lock);
	};
	return hxnull;
}

void* hxthread_test_func_notify_all(hxthread_test_parameters_t* parameters) {
	hxunique_lock lock(*parameters->mutex);
	while(!*parameters->ready) {
		parameters->condition_variable->wait(lock);
	}
	if(parameters->woken) {
		++(*parameters->woken);
	}
	return hxnull;
}

void* hxthread_test_func_lock_unlock_multiple(hxthread_test_parameters_t* parameters) {
	hxunique_lock lock(*parameters->mutex);
	++(*parameters->woken);
	return hxnull;
}

void* hxthread_test_func_wait_notify_sequence(hxthread_test_parameters_t* parameters) {
	hxunique_lock lock(*parameters->mutex);
	while(!*parameters->ready) {
		parameters->condition_variable->wait(lock);
	}
	return hxnull;
}

hxmutex hxthread_local_destructor_mutex;
int hxthread_local_destructor_count = 0;

class hxthread_local_destructor_tracker {
public:
	hxthread_local_destructor_tracker(bool track=false) : track_(track) { }

	~hxthread_local_destructor_tracker() {
		if(track_) {
			hxunique_lock lock(hxthread_local_destructor_mutex);
			++hxthread_local_destructor_count;
		}
	}

	bool track_;
};

hxthread_local<hxthread_local_destructor_tracker> hxthread_local_destructor_tls;

void* hxthread_local_destructor_thread(int*) {
	hxthread_local_destructor_tracker& tracker = hxthread_local_destructor_tls;
	tracker.track_ = true;
	return hxnull;
}

} // namespace {

TEST(hxunique_lock, basic_lock_unlock) {
	hxmutex mutex;
	hxunique_lock lock(mutex);
	EXPECT_TRUE(lock.owns_lock());
	lock.unlock();
	EXPECT_FALSE(lock.owns_lock());
}

TEST(hxmutex, double_lock_unlock) {
	hxmutex mutex;
	EXPECT_TRUE(mutex.lock());
	EXPECT_TRUE(mutex.unlock());
	EXPECT_TRUE(mutex.lock());
	EXPECT_TRUE(mutex.unlock());
}

TEST(hxmutex, native_handle_notnull) {
	hxmutex mutex;
	EXPECT_TRUE(mutex.native_handle() != hxnull);
}

TEST(hxunique_lock, defer_lock) {
	hxmutex mutex;
	hxunique_lock lock(mutex, true);
	EXPECT_FALSE(lock.owns_lock());
	lock.lock();
	EXPECT_TRUE(lock.owns_lock());
}

TEST(hxunique_lock, unlock_without_lock) {
	hxmutex mutex;
	hxunique_lock lock(mutex, true);
	lock.unlock();
	EXPECT_FALSE(lock.owns_lock());
}

TEST(hxunique_lock, lock_twice) {
	hxmutex mutex;
	hxunique_lock lock(mutex, true);
	lock.lock();
	lock.lock();
	EXPECT_TRUE(lock.owns_lock());
}

TEST(hxunique_lock, mutexreference) {
	hxmutex mutex;
	hxunique_lock lock(mutex);
	hxmutex& reference = lock.mutex();
	EXPECT_TRUE(&reference == &mutex);
}

TEST(hxcondition_variable, notify_no_waiters) {
	hxcondition_variable condition_variable;
	EXPECT_TRUE(condition_variable.notify_one());
	EXPECT_TRUE(condition_variable.notify_all());
}

TEST(hxcondition_variable, native_handle_not_null) {
	hxcondition_variable condition_variable;
	EXPECT_TRUE(condition_variable.native_handle() != hxnull);
}

TEST(hxcondition_variable, wait_predicate) {
	hxmutex mutex;
	hxcondition_variable condition_variable;
	hxunique_lock lock(mutex);
	int value = 0;
	condition_variable.wait(lock, hxthread_test_predicate_wait_for_zero(&value));
	SUCCEED();
}

TEST(hxcondition_variable, notify_one_wakes_waiter) {
	hxmutex mutex;
	hxcondition_variable condition_variable;
	bool ready = false;
	hxthread_test_parameters_t parameters(&mutex, &condition_variable, &ready, hxnull);
	hxthread thread(hxthread_test_func_notify_one, &parameters);
	{
		hxunique_lock lock(mutex);
		ready = true;
		condition_variable.notify_one();
	}
	thread.join();
	SUCCEED();
}

TEST(hxcondition_variable, notify_all_wakes_waiters) {
	hxmutex mutex;
	hxcondition_variable condition_variable;
	bool ready = false;
	int woken = 0;
	hxthread_test_parameters_t parameters_tuple1(&mutex, &condition_variable, &ready, &woken);
	hxthread_test_parameters_t parameters_tuple2(&mutex, &condition_variable, &ready, &woken);
	hxthread thread1(hxthread_test_func_notify_all, &parameters_tuple1);
	hxthread thread2(hxthread_test_func_notify_all, &parameters_tuple2);
	{
		hxunique_lock lock(mutex);
		ready = true;
		condition_variable.notify_all();
	}
	thread1.join();
	thread2.join();
	EXPECT_EQ(woken, 2);
}

TEST(hxthread, start_and_join) {
	int shared = 0;
	hxmutex mutex;
	hxthread_test_simple_parameters_t argument = {&mutex, &shared};
	hxthread thread(&hxthread_test_func_increment, &argument);
	EXPECT_TRUE(thread.joinable());
	thread.join();
	EXPECT_FALSE(thread.joinable());
	EXPECT_EQ(shared, 1);
}

TEST(hxthread, multiple_threadsincrement) {
	int shared = 0;
	hxmutex mutex;
	hxthread_test_simple_parameters_t argument1 = {&mutex, &shared};
	hxthread_test_simple_parameters_t argument2 = {&mutex, &shared};
	hxthread thread1(&hxthread_test_func_increment, &argument1);
	hxthread thread2(&hxthread_test_func_increment, &argument2);
	thread1.join();
	thread2.join();
	EXPECT_EQ(shared, 2);
}

TEST(hxmutex, lock_unlock_multiple_threads) {
	hxmutex mutex;
	int shared = 0;
	hxthread_test_parameters_t parameters(&mutex, hxnull, hxnull, &shared);
	hxthread thread1(hxthread_test_func_lock_unlock_multiple, &parameters);
	hxthread thread2(hxthread_test_func_lock_unlock_multiple, &parameters);
	thread1.join();
	thread2.join();
	EXPECT_EQ(shared, 2);
}

TEST(hxunique_lock, ownership_after_unlock) {
	hxmutex mutex;
	hxunique_lock lock(mutex);
	lock.unlock();
	EXPECT_FALSE(lock.owns_lock());
}

TEST(hxunique_lock, ownership_after_lock) {
	hxmutex mutex;
	hxunique_lock lock(mutex, true);
	lock.lock();
	EXPECT_TRUE(lock.owns_lock());
}

TEST(hxthread, join_without_start) {
	hxthread thread;
	// Should not be joinable, so nothing to join.
	EXPECT_FALSE(thread.joinable());
}

TEST(hxunique_lock, multiple_locks) {
	hxmutex mutex1, mutex2;
	hxunique_lock lock1(mutex1);
	hxunique_lock lock2(mutex2);
	EXPECT_TRUE(lock1.owns_lock());
	EXPECT_TRUE(lock2.owns_lock());
}

TEST(hxcondition_variable, wait_notify_sequence) {
	hxmutex mutex;
	hxcondition_variable condition_variable;
	bool ready = false;
	hxthread_test_parameters_t parameters(&mutex, &condition_variable, &ready, hxnull);
	hxthread thread(hxthread_test_func_wait_notify_sequence, &parameters);
	{
		hxunique_lock lock(mutex);
		ready = true;
		condition_variable.notify_one();
	}
	thread.join();
	SUCCEED();
}

TEST(hxthread, multiple_thread_start_join) {
	hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);

	const int reps = 10;
	int shared = 0;
	hxmutex mutex;
	hxthread_test_simple_parameters_t argument = {&mutex, &shared};
	hxthread* threads[reps];
	int i;
	for(i = 0; i < reps; ++i) {
		threads[i] = hxnew<hxthread>(&hxthread_test_func_increment, &argument);
	}
	for(i = 0; i < reps; ++i) {
		threads[i]->join();
		hxdelete(threads[i]);
	}
	EXPECT_EQ(shared, reps);
}

TEST(hxthread_local, destroy_local_runs_on_thread_exit) {
	{
		hxunique_lock lock(hxthread_local_destructor_mutex);
		hxthread_local_destructor_count = 0;
	}

	int dummy=0;
	hxthread thread(&hxthread_local_destructor_thread, &dummy);
	thread.join();

	hxunique_lock lock(hxthread_local_destructor_mutex);
	EXPECT_EQ(hxthread_local_destructor_count, 1);
}

#endif // HX_USE_THREADS
