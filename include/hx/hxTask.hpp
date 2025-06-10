#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>

class hxTaskQueue;

// hxTask - Base class for operations to be performed on a different thread or
// at a later time. Nota bene: While the current allocator is a thread local
// attribute, the memory manager does not support concurrent access to the
// same allocator. Either preallocate working buffers or arrange for locking
// around shared allocators.

class hxTask {
public:
    // Construct task. staticLabel must be a static string.
    // - staticLabel_: A constant string literal or null to label the task.
    inline explicit hxTask(const char* staticLabel_=hxnull)
        : m_nextTask_(hxnull), m_label_(staticLabel_), m_taskQueue_(hxnull) {
    }

    // Destructor for the task. Ensures that the task is not owned by
    // any exclusive owner when deleted.
    virtual ~hxTask() {
        hxAssertRelease(!m_taskQueue_, "deleting queued task: %s", getLabel());
    }

    // Executes the task. This is the main function to be implemented by
    // derived classes. This call is the last time this object is touched by
    // hxTaskQueue. It may delete or re-enqueue itself. Will also be wrapped in
    // hxProfileScope(getLabel());
    // - q_: Pointer to the task queue managing this task.
    // Notes:
    // - This function may delete or re-enqueue the task.
    virtual void execute(hxTaskQueue* q_) = 0;

	// Gets the next task in the linked list.
    // Returns pointer to the next task, or null if there is no next task.
    inline hxTask* getNextTask() const { return m_nextTask_; }

	// Sets the next task in the linked list.
    // - x_: Pointer to the task to set as the next task.
    inline void setNextTask(hxTask* x_) { m_nextTask_ = x_; }

	// Gets the label of the task.
    // Returns the label of the task, or "task" if no label is set.
    inline const char* getLabel() const { return m_label_ ? m_label_ : "task"; }

	// Sets the label of the task.
    // - x_: A constant string literal or null to set as the task label.
    inline void setLabel(const char* x_) { m_label_ = x_; }

	// Sets the task queue which is to be the exclusive owner of the task.
    // - x_: Pointer to the new exclusive owner, or null to clear ownership.
    // Notes:
    // - Ensures that the task is not re-enqueued while already owned.
    // - The task must not be part of a linked list when setting ownership.
    inline void setTaskQueue(hxTaskQueue* x_) {
        hxAssertRelease((!m_taskQueue_ || !x_) && !m_nextTask_, "re-enqueuing task: %s", getLabel());
        m_taskQueue_ = x_;
    }

private:
    // Copy constructor is deleted to prevent copying of tasks.
    hxTask(const hxTask&) HX_DELETE_FN;

    // Assignment operator is deleted to prevent copying of tasks.
    void operator=(const hxTask&) HX_DELETE_FN;

    // Pointer to the next task in the linked list.
    hxTask* m_nextTask_;

    // Label for the task, typically a static string literal.
    const char* m_label_;

    // Pointer to the exclusive owner of the task, if any.
    hxTaskQueue* m_taskQueue_;
};
