#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>

class hxtask_queue;

/// `hxtask` - Base class for operations to be performed on a different thread or
/// at a later time. Nota bene: While the current allocator is a thread local
/// attribute, the memory manager does not support concurrent access to the
/// same allocator. Either preallocate working buffers or arrange for locking
/// around shared allocators.
class hxtask {
public:
	/// Construct task. `static_label` must be a static string.
	/// - `static_label` : A constant string literal or `null` to label the task.
	inline explicit hxtask(const char* static_label_=hxnull)
		: m_next_task_(hxnull), m_label_(static_label_), m_task_queue_(hxnull) {
	}

	/// Destructor for the task. Ensures that the task is not owned by
	/// any exclusive owner when deleted.
	virtual ~hxtask(void) {
		hxassertrelease(!m_task_queue_, "deleting_queued_task %s", get_label());
	}

	/// Executes the task. This is the main function to be implemented by
	/// derived classes. This call is the last time this object is touched by
	/// hxtask_queue. It may delete or re-enqueue itself. Will also be wrapped
	/// in `hxprofile_scope(get_label());` This function may delete or re-enqueue
	/// the task.
	/// - `q` : Pointer to the task queue managing this task.
	virtual void execute(hxtask_queue* q_) = 0;

	/// Returns pointer to the next task, or null if there is no next task.
	inline hxtask* get_next_task(void) const { return m_next_task_; }

	/// Sets the next task in the linked list.
	/// - `x` : Pointer to the task to set as the next task.
	inline void set_next_task(hxtask* x_) { m_next_task_ = x_; }

	/// Returns the label of the task, or "task" if no label is set.
	inline const char* get_label(void) const { return m_label_ ? m_label_ : "task"; }

	/// Sets the label of the task.
	/// - `x` : A constant string literal or null to set as the task label.
	inline void set_label(const char* x_) { m_label_ = x_; }

	/// Sets the task queue which is to be the exclusive owner of the task.
	/// - `x` : Pointer to the new exclusive owner, or null to clear ownership.
	inline void set_task_queue(hxtask_queue* x_) {
		hxassertrelease((!m_task_queue_ || !x_) && !m_next_task_, "reenqueuing_task %s", get_label());
		m_task_queue_ = x_;
	}

private:
	// Copy constructor is deleted to prevent copying of tasks.
	hxtask(const hxtask&) hxdelete_fn;

	// Assignment operator is deleted to prevent copying of tasks.
	void operator=(const hxtask&) hxdelete_fn;

	// Pointer to the next task in the linked list.
	hxtask* m_next_task_;

	// Label for the task, typically a static string literal.
	const char* m_label_;

	// Pointer to the exclusive owner of the task, if any.
	hxtask_queue* m_task_queue_;
};
