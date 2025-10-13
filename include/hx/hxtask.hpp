#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include "hatchling.h"

class hxtask_queue;

/// `hxtask` - Base class for operations to be performed on a different thread or
/// at a later time.
class hxtask {
public:
	/// Constructs a task. `static_label` must be a static string.
	/// - `static_label` : A constant string literal or `null` to label the task.
	/// - `priority` : Larger values run sooner when queued.
	explicit hxtask(const char* static_label_=hxnull, int priority_=0)
		: m_label_(static_label_), m_task_queue_(hxnull), m_priority_(priority_) {
	}

	/// Destroys the task. Ensures that the task is not owned by any exclusive
	/// owner when deleted.
	virtual ~hxtask(void) {
		hxassertrelease(!m_task_queue_, "deleting_queued_task %s", this->get_label());
	}

	/// Executes the task. This is the main function to implement in derived
	/// classes. This call is the last time this object is touched by
	/// `hxtask_queue`. The function may delete or re-enqueue the task and is also
	/// wrapped in `hxprofile_scope(get_label());`
	/// - `q` : Pointer to the task queue managing this task.
	virtual void execute(hxtask_queue* q_) = 0;

	/// Returns the label of the task, or `"task"` if no label is set.
	const char* get_label(void) const { return m_label_ ? m_label_ : "task"; }

	/// Sets the label of the task.
	/// - `x` : A constant string literal or null.
	void set_label(const char* x_) { m_label_ = x_; }

	/// Returns the priority of the task. Higher values run sooner.
	int get_priority(void) const { return m_priority_; }

	/// Sets the priority of the task. Higher values run sooner.
	/// - `x` : The new priority value.
	void set_priority(int x_) { m_priority_ = x_; }

	/// Sets the task queue that is to be the exclusive owner of the task.
	/// - `x` : Pointer to the new exclusive owner, or null to clear ownership.
	void set_task_queue(hxtask_queue* x_) {
		hxassertrelease((!m_task_queue_ || !x_), "reenqueuing_task %s", this->get_label());
		m_task_queue_ = x_;
	}

private:
	// Copy constructor is deleted to prevent copying of tasks.
	hxtask(const hxtask&) = delete;

	// Assignment operator is deleted to prevent copying of tasks.
	void operator=(const hxtask&) = delete;

	// Label for the task, typically a static string literal.
	const char* m_label_;

	// Pointer to the exclusive owner of the task, if any.
	hxtask_queue* m_task_queue_;

	// Priority used by `hxtask_queue` when scheduling.
	int m_priority_;
};

/// `hxkey_less(const hxtask&, const hxtask&)` - Orders tasks by priority for
/// use with `hxarray::push_heap` / `pop_heap`.
inline bool hxkey_less(const hxtask& a_, const hxtask& b_) {
	return a_.get_priority() < b_.get_priority();
}
