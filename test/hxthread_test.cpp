// Copyright 2017-2025 Adrian Johnston

#include <hx/hxthread.hpp>
#include <hx/hxtest.hpp>

#if HX_USE_THREADS

namespace hxx_ {

struct hxthread_test_simple_parameters_t_ {
    hxmutex* mutex_;
    int* shared_;
};

class hxthread_test_parameters_t_ {
public:
    hxmutex* mutex_;
    hxcondition_variable* condition_variable_;
    bool* ready_;
    int* woken_;

    hxthread_test_parameters_t_(hxmutex* mutex, hxcondition_variable* condition_variable,
                                bool* ready, int* woken)
        : mutex_(mutex), condition_variable_(condition_variable), ready_(ready), woken_(woken) { }
};

class hxthread_test_predicate_wait_for_zero {
public:
    explicit hxthread_test_predicate_wait_for_zero(int* v) : value_(v) {}
    bool operator()() const { return *value_ == 0; }
    int* value_;
};

void* hxthread_test_func_increment_(hxthread_test_simple_parameters_t_* parameters_) {
    hxunique_lock lock(*parameters_->mutex_);
    ++(*parameters_->shared_);
    return hxnull;
}

void* hxthread_test_func_notify_one(hxthread_test_parameters_t_* parameters_) {
    hxunique_lock lock(*parameters_->mutex_);
    while (!*parameters_->ready_) {
        parameters_->condition_variable_->wait(lock);
    };
    return hxnull;
}

void* hxthread_test_func_notify_all(hxthread_test_parameters_t_* parameters_) {
    hxunique_lock lock(*parameters_->mutex_);
    while (!*parameters_->ready_) {
        parameters_->condition_variable_->wait(lock);
    }
    if (parameters_->woken_) {
        ++(*parameters_->woken_);
    }
    return hxnull;
}

void* hxthread_test_func_lock_unlock_multiple(hxthread_test_parameters_t_* parameters_) {
    hxunique_lock lock(*parameters_->mutex_);
    ++(*parameters_->woken_);
    return hxnull;
}

void* hxthread_test_func_wait_notify_sequence(hxthread_test_parameters_t_* parameters_) {
    hxunique_lock lock(*parameters_->mutex_);
    while (!*parameters_->ready_) {
        parameters_->condition_variable_->wait(lock);
    }
    return hxnull;
}

} // using hxx_

TEST(hxunique_lock, BasicLockUnlock) {
    hxmutex mutex_;
    hxunique_lock lock(mutex_);
    EXPECT_TRUE(lock.owns_lock());
    lock.unlock();
    EXPECT_FALSE(lock.owns_lock());
}

TEST(hxmutex, DoubleLockUnlock) {
    hxmutex mutex_;
    EXPECT_TRUE(mutex_.lock());
    EXPECT_TRUE(mutex_.unlock());
    EXPECT_TRUE(mutex_.lock());
    EXPECT_TRUE(mutex_.unlock());
}

TEST(hxmutex, NativeHandleNotNull) {
    hxmutex mutex_;
    EXPECT_TRUE(mutex_.native_handle() != hxnull);
}

TEST(hxunique_lock, DeferLock) {
    hxmutex mutex_;
    hxunique_lock lock(mutex_, true);
    EXPECT_FALSE(lock.owns_lock());
    lock.lock();
    EXPECT_TRUE(lock.owns_lock());
}

TEST(hxunique_lock, UnlockWithoutLock) {
    hxmutex mutex_;
    hxunique_lock lock(mutex_, true);
    lock.unlock();
    EXPECT_FALSE(lock.owns_lock());
}

TEST(hxunique_lock, LockTwice) {
    hxmutex mutex_;
    hxunique_lock lock(mutex_, true);
    lock.lock();
    lock.lock();
    EXPECT_TRUE(lock.owns_lock());
}

TEST(hxunique_lock, MutexReference) {
    hxmutex mutex_;
    hxunique_lock lock(mutex_);
    hxmutex& reference_ = lock.mutex();
    EXPECT_TRUE(&reference_ == &mutex_);
}

TEST(hxcondition_variable, NotifyNoWaiters) {
    hxcondition_variable condition_variable_;
    EXPECT_TRUE(condition_variable_.notify_one());
    EXPECT_TRUE(condition_variable_.notify_all());
}

TEST(hxcondition_variable, NativeHandleNotNull) {
    hxcondition_variable condition_variable_;
    EXPECT_TRUE(condition_variable_.native_handle() != hxnull);
}

TEST(hxcondition_variable, WaitPredicate) {
    hxmutex mutex_;
    hxcondition_variable condition_variable_;
    hxunique_lock lock(mutex_);
    int value_ = 0;
    condition_variable_.wait(lock, hxthread_test_predicate_wait_for_zero(&value_));
    SUCCEED();
}

TEST(hxcondition_variable, NotifyOneWakesWaiter) {
    hxmutex mutex_;
    hxcondition_variable condition_variable_;
    bool ready_ = false;
    hxthread_test_parameters_t_ parameters_(&mutex_, &condition_variable_, &ready_, hxnull);
    hxthread thread_(hxthread_test_func_notify_one, &parameters_);
    {
        hxunique_lock lock(mutex_);
        ready_ = true;
        condition_variable_.notify_one();
    }
    thread_.join();
    SUCCEED();
}

TEST(hxcondition_variable, NotifyAllWakesWaiters) {
    hxmutex mutex_;
    hxcondition_variable condition_variable_;
    bool ready_ = false;
    int woken_ = 0;
    hxthread_test_parameters_t_ parameters_tuple1_(&mutex_, &condition_variable_, &ready_, &woken_);
    hxthread_test_parameters_t_ parameters_tuple2_(&mutex_, &condition_variable_, &ready_, &woken_);
    hxthread thread1_(hxthread_test_func_notify_all, &parameters_tuple1_);
    hxthread thread2_(hxthread_test_func_notify_all, &parameters_tuple2_);
    {
        hxunique_lock lock(mutex_);
        ready_ = true;
        condition_variable_.notify_all();
    }
    thread1_.join();
    thread2_.join();
    EXPECT_EQ(woken_, 2);
}

TEST(hxthread, StartAndJoin) {
    int shared_ = 0;
    hxmutex mutex_;
    hxthread_test_simple_parameters_t_ argument_ = {&mutex_, &shared_};
    hxthread thread_(&hxthread_test_func_increment_, &argument_);
    EXPECT_TRUE(thread_.joinable());
    thread_.join();
    EXPECT_FALSE(thread_.joinable());
    EXPECT_EQ(shared_, 1);
}

TEST(hxthread, MultipleThreadsIncrement) {
    int shared_ = 0;
    hxmutex mutex_;
    hxthread_test_simple_parameters_t_ argument1_ = {&mutex_, &shared_};
    hxthread_test_simple_parameters_t_ argument2_ = {&mutex_, &shared_};
    hxthread thread1_(&hxthread_test_func_increment_, &argument1_);
    hxthread thread2_(&hxthread_test_func_increment_, &argument2_);
    thread1_.join();
    thread2_.join();
    EXPECT_EQ(shared_, 2);
}

TEST(hxmutex, LockUnlockMultipleThreads) {
    hxmutex mutex_;
    int shared_ = 0;
    hxthread_test_parameters_t_ parameters_(&mutex_, hxnull, hxnull, &shared_);
    hxthread thread1_(hxthread_test_func_lock_unlock_multiple, &parameters_);
    hxthread thread2_(hxthread_test_func_lock_unlock_multiple, &parameters_);
    thread1_.join();
    thread2_.join();
    EXPECT_EQ(shared_, 2);
}

TEST(hxunique_lock, OwnershipAfterUnlock) {
    hxmutex mutex_;
    hxunique_lock lock(mutex_);
    lock.unlock();
    EXPECT_FALSE(lock.owns_lock());
}

TEST(hxunique_lock, OwnershipAfterLock) {
    hxmutex mutex_;
    hxunique_lock lock(mutex_, true);
    lock.lock();
    EXPECT_TRUE(lock.owns_lock());
}

TEST(hxthread, JoinWithoutStart) {
    hxthread thread_;
    // Should not be joinable, so nothing to join
    EXPECT_FALSE(thread_.joinable());
}

TEST(hxunique_lock, MultipleLocks) {
    hxmutex mutex1_, mutex2_;
    hxunique_lock lock1(mutex1_);
    hxunique_lock lock2(mutex2_);
    EXPECT_TRUE(lock1.owns_lock());
    EXPECT_TRUE(lock2.owns_lock());
}

TEST(hxcondition_variable, WaitNotifySequence) {
    hxmutex mutex_;
    hxcondition_variable condition_variable_;
    bool ready_ = false;
    hxthread_test_parameters_t_ parameters_(&mutex_, &condition_variable_, &ready_, hxnull);
    hxthread thread_(hxthread_test_func_wait_notify_sequence, &parameters_);
    {
        hxunique_lock lock(mutex_);
        ready_ = true;
        condition_variable_.notify_one();
    }
    thread_.join();
    SUCCEED();
}

TEST(hxthread, MultipleThreadStartJoin) {
    const int reps_ = 10;
    int shared_ = 0;
    hxmutex mutex_;
    hxthread_test_simple_parameters_t_ argument_ = {&mutex_, &shared_};
    hxthread* threads_[reps_];
    int i_;
    for (i_ = 0; i_ < reps_; ++i_) {
        threads_[i_] = new hxthread(&hxthread_test_func_increment_, &argument_);
    }
    for (i_ = 0; i_ < reps_; ++i_) {
        threads_[i_]->join();
        delete threads_[i_];
    }
    EXPECT_EQ(shared_, reps_);
}

#endif // HX_USE_THREADS
