#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>
#include <hx/hxTime.h>

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
	HX_INLINE explicit hxTask(const char* staticLabel_=hxnull)
		: m_nextTask(hxnull), m_label(staticLabel_), m_owner(hxnull) {
	}

	// Delete task.  The execute() call may free task _if allocator is thread safe_.
	HX_INLINE virtual ~hxTask() { hxAssertRelease(!m_owner, "deleting queued task: %s", getLabel()); }

	// Will be wrapped in hxProfileScope(getLabel());
	virtual void execute(hxTaskQueue* q_) = 0;

	// Embedded linked list of tasks used by owners.
	HX_INLINE hxTask* getNextTask() const { return m_nextTask; }
	HX_INLINE void setNextTask(hxTask* x_) { m_nextTask = x_; }

	HX_INLINE const char* getLabel() const { return m_label ? m_label : "task"; }
	HX_INLINE void setLabel(const char* x_) { m_label = x_; }

	// Enforces a single ownership policy.  Must set to hxnull first.
	HX_INLINE void setOwner(const void* x_) {
		hxAssertRelease((!m_owner || !x_) && !m_nextTask, "re-enqueuing task: %s", getLabel());
		m_owner = x_;
	}

private:
	hxTask(const hxTask&); // = delete
	void operator=(const hxTask&); // = delete

	hxTask* m_nextTask;
	const char* m_label;
	const void* m_owner;
};
