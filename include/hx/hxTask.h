#pragma once
// Copyright 2017-2019 Adrian Johnston

#include <hx/hatchling.h>
#include <hx/hxTime.h>

class hxTaskQueue;

// ----------------------------------------------------------------------------

class hxTask {
public:
	// Construct task.  staticLabel must be a static string.
	HX_INLINE explicit hxTask(const char* staticLabel = hxnull)
		: m_nextTask(hxnull), m_label(staticLabel), m_owner(hxnull) {
	}

	// Delete task.  The execute() call may free task _if allocator is thread safe_.
	HX_INLINE virtual ~hxTask() { hxAssertRelease(!m_owner, "deleting queued task: %s", getLabel()); }

	// Will be wrapped in hxProfileScope(getLabel());
	virtual void execute(hxTaskQueue* q) = 0;

	// Embedded linked list of tasks used by owners.
	HX_INLINE hxTask* getNextTask() const { return m_nextTask; }
	HX_INLINE void setNextTask(hxTask* x) { m_nextTask = x; }

	HX_INLINE const char* getLabel() const { return m_label ? m_label : "task"; }
	HX_INLINE void setLabel(const char* x) { m_label = x; }

	// Enforces a single ownership policy.  Must set to hxnull before assigning a new owner, 
	HX_INLINE void setOwner(const void* x) {
		hxAssertRelease((!m_owner || !x) && !m_nextTask, "re-enqueuing task: %s", getLabel());
		m_owner = x;
	}

private:
	hxTask(const hxTask&); // = delete
	void operator=(const hxTask&); // = delete

	hxTask* m_nextTask;
	const char* m_label;
	const void* m_owner;
};
