#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include "hatchling.h"

class hxtask_queue;

/// `hxtask` - Pure virtual base class for operations to be performed on a
/// different thread or at a later time.
class hxtask {
public:
	/// A virtual destructor.
	virtual ~hxtask(void) { }

	/// Executes the task. This is the main function to implement in derived
	/// classes. This call is the last time this object is touched by the
	/// `hxtask_queue`. An `execute` override may delete or re-enqueue the
	/// `this` pointer. It is also wrapped in a `hxprofiler` scope when called.
	/// - `q` : Pointer to the task queue managing this task.
	virtual void execute(hxtask_queue* q_) = 0;

	/// Returns the label of the task, or `"task"` by default.
	virtual const char* get_label(void) const { return "task"; }
};
