#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>
#include <hx/hxTime.hpp>

class hxTaskQueue;

// ----------------------------------------------------------------------------
// hxTask.  Base class for operations to be performed on a different thread or
// at a later time.  Nota bene: While the current allocator is a thread local
// attribute, the memory manager does not support concurrent access to the
// same allocator.  Either preallocate working buffers or arrange for locking
// around shared allocators.  

class hxTask {
public:
	// Construct task.  staticLabel must be a static string.
	// Parameters:
	// - staticLabel_: A constant string literal or null to label the task.
	HX_INLINE explicit hxTask(const char* staticLabel_=hxnull)
		: m_nextTask(hxnull), m_label(staticLabel_), m_exclusiveOwner(hxnull) {
	}

	// Destructor for the task.
	// Ensures that the task is not owned by any exclusive owner when deleted.
	HX_INLINE virtual ~hxTask() { 
		hxAssertRelease(!m_exclusiveOwner, "deleting queued task: %s", getLabel()); 
	}

	// Executes the task. This is the main function to be implemented by derived classes.
	// This call is the last time this object is touched by hxTaskQueue.  It may
	// delete or re-enqueue itself.  Will also be wrapped in hxProfileScope(getLabel());
	// Parameters:
	// - q_: Pointer to the task queue managing this task.
	// Notes:
	// - This function may delete or re-enqueue the task.
	virtual void execute(hxTaskQueue* q_) = 0;

	// Gets the next task in the linked list.
	// Returns:
	// - Pointer to the next task, or null if there is no next task.
	HX_INLINE hxTask* getNextTask() const { return m_nextTask; }

	// Sets the next task in the linked list.
	// Parameters:
	// - x_: Pointer to the task to set as the next task.
	HX_INLINE void setNextTask(hxTask* x_) { m_nextTask = x_; }

	// Gets the label of the task.
	// Returns:
	// - The label of the task, or "task" if no label is set.
	HX_INLINE const char* getLabel() const { return m_label ? m_label : "task"; }

	// Sets the label of the task.
	// Parameters:
	// - x_: A constant string literal or null to set as the task label.
	HX_INLINE void setLabel(const char* x_) { m_label = x_; }

	// Sets the exclusive owner of the task.
	// Parameters:
	// - x_: Pointer to the new exclusive owner, or null to clear ownership.
	// Notes:
	// - Ensures that the task is not re-enqueued while already owned.
	// - The task must not be part of a linked list when setting ownership.
	HX_INLINE void setExclusiveOwner(const void* x_) {
		hxAssertRelease((!m_exclusiveOwner || !x_) && !m_nextTask, "re-enqueuing task: %s", getLabel());
		m_exclusiveOwner = x_;
	}

private:
	// Copy constructor is deleted to prevent copying of tasks.
	hxTask(const hxTask&); // = delete

	// Assignment operator is deleted to prevent copying of tasks.
	void operator=(const hxTask&); // = delete

	// Pointer to the next task in the linked list.
	hxTask* m_nextTask;

	// Label for the task, typically a static string literal.
	const char* m_label;

	// Pointer to the exclusive owner of the task, if any.
	const void* m_exclusiveOwner;
};
