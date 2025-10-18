#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

/// \file hx/hxhash_table.hpp Implements a hash table that operates without
/// reallocating memory or copying data. Visualizing how a memory allocator
/// has to serve doubling hash table sizes shows how quickly memory fragments. So
/// this implementation expects you to allocate the largest table you may need
/// in advance. Each bucket uses an embedded linked list. Hash tables can act as
/// either an unordered map or an unordered set and support operations that
/// allow for unique or duplicate keys. While this interface is designed to feel
/// familiar, code using standard containers will need some adjustments. In
/// particular, all table modifications are non-standard.
///
/// Note that any node `T` using key `K` will work as long as it has the following
/// fields and `K` has an `operator==` or an `hxkey_equal` overload.
/// ```
/// class T {
///   using key_t = K;			// Tell the hash table what key to use.
///   T(key_t);					// Construct from key. e.g., for operator[].
///   void*& hash_next();		// Used by hxhash_table for an embedded linked list.
///   void* hash_next() const;	// Constant version of hash_next.
///   const key_t& key() const;	// Returns key constructed with.
///   hxhash_t hash() const;	// Returns hash of key constructed with.
/// };
/// ```
/// `hxhash_table_set_node` and `hxhash_table_map_node` are provided and
/// recommended as replacements for `std::unordered_set` and
/// `std::unordered_map`. Custom key types will require either an `operator==`
/// or an `hxkey_equal` overload and will require an `hxkey_hash` overload.
///
/// They might be used as follows:
/// ```
/// // An unordered set of allowed or blocked internet addresses.
/// using ipv6_set_t = hxhash_table<hxhash_table_set_node<ipv6_address_t>>;
///
/// // A fixed-size unordered map of material identifiers to material
/// // properties. Missing materials can be safely resolved.
/// using material_db_t = hxhash_table<hxhash_table_map_node<material_id_t, material_t>, 1024>;
/// ```
/// `hx/hxhash_table_nodes.hpp` also provides specializations of the `hxhash_table`
/// `node_t` template parameter for integers and strings.

#include "detail/hxhash_table_detail.hpp"
#include "hxkey.hpp"
#include "hxutility.h"

#if HX_CPLUSPLUS >= 202002L
/// Concept capturing the interface requirements for `hxhash_table` nodes.
template<typename node_t_>
concept hxhash_table_concept_ =
	requires { typename node_t_::key_t; } &&
	requires(node_t_& node_, const node_t_& const_node_) {
		{ node_.hash_next() = (void*)hxnull } -> hxconvertible_to<void*&>;
		{ const_node_.hash_next() } -> hxconvertible_to<void*>;
		{ const_node_.key() } -> hxconvertible_to<const typename node_t_::key_t&>;
		{ const_node_.hash() } -> hxconvertible_to<hxhash_t>;
	};
#else
#define hxhash_table_concept_ typename
#endif

// XXX: Move to a d-list and implement tbl::erase(node_t*)?

/// `hxhash_table_set_node` - Optional base class for unordered set entries.
/// Caches the hash value. Copying and modification are disallowed to protect
/// the integrity of the hash table. See `hxhash_table_map_node` if you need a
/// mutable node. The hash table uses duck typing, so only the interface is
/// required.
template<typename key_t_>
class hxhash_table_set_node {
public:
	using key_t = key_t_;

	template<typename ref_t_>
	hxhash_table_set_node(ref_t_&& key_)
		: m_hash_next_(hxnull), m_key_(hxforward<ref_t_>(key_))
	{
		// You need to implement hxkey_hash for your key_t_ type.
		m_hash_ = hxkey_hash(m_key_);
	}

	/// Boilerplate required by `hxhash_table`.
	void* hash_next(void) const { return m_hash_next_; }
	void*& hash_next(void) { return m_hash_next_; }

	/// The key and hash identify the `node_t` and should not change once added.
	const key_t_& key(void) const { return m_key_; }

	/// Hash values are not required to be unique.
	hxhash_t hash(void) const { return m_hash_; };

private:
	hxhash_table_set_node(void) = delete;
	// m_hash_next_ should not be copied.
	hxhash_table_set_node(const hxhash_table_set_node&) = delete;
	void operator=(const hxhash_table_set_node&) = delete;

	// The hash table uses m_hash_next_ to implement an embedded linked list.
	void* m_hash_next_;
	key_t_ m_key_;
	hxhash_t m_hash_;
};

/// Base class for unordered map entries.
template<typename key_t_, typename value_t_>
class hxhash_table_map_node : public hxhash_table_set_node<key_t_> {
public:
	using key_t = key_t_;
	using value_t = value_t_;

	// `value_t` must default-construct when using `operator[]`.
	hxhash_table_map_node(const key_t_& key_) :
		hxhash_table_set_node<key_t_>(key_) { }

	template<typename ref_t_>
	hxhash_table_map_node(const key_t_& key_, ref_t_&& value_) :
		hxhash_table_set_node<key_t_>(key_), m_value_(hxforward<ref_t_>(value_)) { }

	const value_t_& value(void) const { return m_value_; }
	value_t_& value(void) { return m_value_; }
private:
	value_t_ m_value_;
};

/// `hxhash_table` - See the top of this file for a description.
///
/// `node_t` must be a subclass of `hxhash_table_node` with the interface
/// described above. If non-zero, `table_size_bits` configures the hash table
/// size to `2^table_size_bits`. Otherwise use `set_table_size_bits` to configure
/// hash bits dynamically. See `hxdo_not_delete` for situations where the table
/// does not own the nodes.
template<hxhash_table_concept_ node_t_,
	hxhash_t table_size_bits_=hxallocator_dynamic_capacity,
	typename deleter_t_=hxdefault_delete>
class hxhash_table {
public:
	using node_t = node_t_;
	using key_t = typename node_t_::key_t;

	/// A forward iterator. Iteration is Θ(`n + (1 << table_size_bits)`).
	/// Iterators are only invalidated by the removal of the `node_t`
	/// referenced.
	class const_iterator
	{
	public:
		/// Constructs an iterator pointing to the beginning of the hash table.
		const_iterator(const hxhash_table* table_)
			: m_hash_table_(const_cast<hxhash_table*>(table_)), m_next_index_(0u), m_current_node_(hxnull) { next_bucket(); }

		/// Constructs an iterator pointing to the end of the hash table.
		const_iterator(void) : m_hash_table_(hxnull), m_next_index_(0u), m_current_node_(hxnull) { } // end

		/// Advances the iterator to the next element.
		const_iterator& operator++(void) {
			hxassertmsg(m_current_node_, "invalid_iterator"); // !end
			if(!(m_current_node_ = (node_t_*)m_current_node_->hash_next())) {
				this->next_bucket();
			}
			return *this;
		}

		/// Advances the iterator to the next element (post-increment).
		const_iterator operator++(int) { const_iterator t_(*this); operator++(); return t_; }

		/// Compares two iterators for equality.
		bool operator==(const const_iterator& x_) const { return m_current_node_ == x_.m_current_node_; }

		/// Compares two iterators for inequality.
		bool operator!=(const const_iterator& x_) const { return m_current_node_ != x_.m_current_node_; }

		/// Dereferences the iterator to access the current `node_t`.
		const node_t_& operator*(void) const { return *m_current_node_; }

		/// Dereferences the iterator to access the current `node_t`'s pointer.
		const node_t_* operator->(void) const { return m_current_node_; }

	private:
		/// Advances the iterator to the next non-empty bucket.
		void next_bucket(void) {
			hxassertmsg(m_hash_table_ && !m_current_node_, "invalid_iterator");
			while(m_next_index_ < m_hash_table_->m_table_.capacity()) {
				if(node_t_* n_ = m_hash_table_->m_table_.data()[m_next_index_++]) {
					m_current_node_ = n_;
					return;
				}
			}
		}

		hxhash_table* m_hash_table_;
		hxhash_t m_next_index_;

	protected:
		// Used by const_iterator.
		/// \cond HIDDEN
		node_t_* m_current_node_;
		/// \endcond
	};

	/// A mutable iterator that can modify the elements of the hash table.
	class iterator : public const_iterator
	{
	public:
		/// Constructs an iterator pointing to the beginning of the hash table.
		iterator(hxhash_table* tbl_) : const_iterator(tbl_) { }

		/// Constructs an iterator pointing to the end of the hash table.
		iterator(void) { }

		/// Advances the iterator to the next element.
		iterator& operator++(void) { const_iterator::operator++(); return *this; }

		/// Advances the iterator to the next element (post-increment).
		iterator operator++(int) { iterator cit_(*this); const_iterator::operator++(); return cit_; }

		/// Dereferences the iterator to access the current `node_t`.
		node_t_& operator*(void) const { return *this->m_current_node_; }

		/// Dereferences the iterator to access the current `node_t`'s pointer.
		node_t_* operator->(void) const { return this->m_current_node_; }
	};

	/// Constructs an empty hash table with a capacity of `table_size_bits^2`.
	explicit hxhash_table(void) { m_size_ = 0u; }

	/// Destructs the hash table and deletes all resources.
	~hxhash_table(void) { this->clear(); }

	/// Sets the number of hash bits and allocate memory for the table. (only
	/// for dynamic capacity).
	/// - `bits` : The number of hash bits to set for the hash table.
	void set_table_size_bits(hxhash_t bits_) { m_table_.set_table_size_bits(bits_); };

	/// Returns a const iterator pointing to the beginning of the hash table.
	const_iterator begin(void) const { return const_iterator(this); }

	/// Returns an iterator pointing to the beginning of the hash table.
	iterator begin(void) { return iterator(this); }

	/// Returns a const iterator pointing to the beginning of the hash table.
	const_iterator cbegin(void) const { return const_iterator(this); }

	/// Returns a const iterator pointing to the beginning of the hash table.
	const_iterator cbegin(void) { return const_iterator(this); }

	/// Returns a const iterator pointing to the end of the hash table.
	const_iterator end(void) const { return const_iterator(); }

	/// Returns an iterator pointing to the end of the hash table.
	iterator end(void) { return iterator(); }

	/// Returns a const iterator pointing to the end of the hash table.
	const_iterator cend(void) const { return const_iterator(); }

	/// Returns a const iterator pointing to the end of the hash table.
	const_iterator cend(void) { return const_iterator(); }

	/// Returns the number of elements in the hash table.
	size_t size(void) const { return m_size_; }

	/// Checks if the hash table is empty.
	bool empty(void) const { return m_size_ == 0u; }

	/// Returns a node containing key if any or allocates and returns a new one.
	/// Any allocation required uses `hxsystem_allocator_current` and `HX_ALIGNMENT`.
	/// - `key` : The key to search for or insert.
	node_t_& operator[](const typename node_t_::key_t& key_) { return this->insert_unique(key_); }

	/// Returns a node containing key if any or allocates and returns a new one.
	/// Unfortunately this code may calculate the hash twice.
	/// - `key` : The key to search for or insert.
	/// - `allocator` : The memory manager ID to use for allocation. Defaults to `hxsystem_allocator_current`.
	/// - `alignment` : The alignment for allocation. Defaults to `HX_ALIGNMENT`.
	node_t_& insert_unique(const typename node_t_::key_t& key_,
		hxsystem_allocator_t allocator_=hxsystem_allocator_current,
		hxalignment_t alignment_=HX_ALIGNMENT);

	/// Inserts a `node_t` into the hash table, allowing duplicate keys. Nodes that
	/// have non-null hash pointers are allowed because they may have been released
	/// from a table that way.
	/// - `node` : The `node_t` to insert into the hash table.
	void insert_node(node_t_* ptr_);

	/// Returns a `node_t` matching key if any. If previous is non-null it must be
	/// a node previously returned from `find()` with the same key and that has not
	/// been removed. Then `find()` will return a subsequent node if any.
	/// The previous object is non-const as it may be modified.
	/// - `key` : The key to search for in the hash table.
	/// - `previous` : A previously found `node_t` with the same key, or nullptr.
	node_t_* find(const typename node_t_::key_t& key_, const node_t_* previous_=hxnull);

	/// `const` version.
	const node_t_* find(const typename node_t_::key_t& key_, const node_t_* previous_=hxnull) const {
		// This code calls the non-const version for brevity.
		return const_cast<hxhash_table*>(this)->find(key_, previous_);
	}

	/// Counts the number of Nodes with the given key.
	/// - `key` : The key to count occurrences of in the hash table.
	size_t count(const typename node_t_::key_t& key_) const;

	/// Removes and returns the first `node_t` with the given key.
	/// - `key` : The key to search for and remove from the hash table.
	node_t_* extract(const typename node_t_::key_t& key_);

	/// Releases all Nodes matching key and calls `deleter` on every node. Returns
	/// the number of nodes released. Deleter can be functions with signature `void
	/// deleter(node_t*)` and functors supporting `operator()(node_t*)` and with an
	/// `operator bool`. e.g., a free list or a null pointer.
	/// - `key` : The key to search for and remove from the hash table.
	/// - `deleter` : A function or functor to call on each removed `node_t`.
	template<typename deleter_override_t_>
	size_t erase(const typename node_t_::key_t& key_, const deleter_override_t_& deleter_);

	/// Removes and calls `deleter_t::operator()` on nodes with an equivalent key.
	size_t erase(const typename node_t_::key_t& key_) {
		return this->erase(key_, deleter_t_());
	}

	/// Removes all Nodes matching the given key without deleting them.
	size_t release_key(const typename node_t_::key_t& key_) {
		// Pass a null pointer for the deleter. Just to show off.
		return this->erase(key_, (void(*)(node_t_*))0);
	}

	/// Removes all nodes and calls `deleter()` on every node. Deleter can be
	/// function pointers with signature `void deleter(node_t*)` or functors
	/// supporting `operator()(node_t*)` and `operator (bool)`. deleter could be
	/// a free list or a null function pointer.
	/// - `deleter` : A function or functor to call on each removed `node_t`.
	template<typename deleter_override_t_>
	void clear(const deleter_override_t_& deleter_);

	/// Removes all nodes and calls `deleter_t::operator()` on every node.
	void clear(void) { this->clear(deleter_t_()); }

	/// Clears the hash table without deleting any Nodes.
	void release_all(void);

	/// Returns the number of buckets in the hash table.
	size_t bucket_count(void) const { return m_table_.capacity(); };

	/// Returns the average number of Nodes per bucket.
	float load_factor(void) const {
		return m_table_.capacity() ? ((float)m_size_ / (float)m_table_.capacity()) : 0u;
	}

	/// Returns the size of the largest bucket.
	size_t load_max(void) const;

private:
	static_assert(table_size_bits_ < hxhash_bits, "Hash bits must be [0..hxhash_bits].");

	// Not ideal.
	hxhash_table(const hxhash_table&) = delete;

	// Pointer to head of singly-linked list for key's hash value.
	node_t_** get_bucket_head_(hxhash_t hash_);

	const node_t_*const* get_bucket_head_(hxhash_t hash_) const;

	size_t m_size_;
	hxhash_table_internal_allocator_<node_t_, table_size_bits_> m_table_;
};

template<hxhash_table_concept_ node_t_, hxhash_t table_size_bits_, typename deleter_t_>
inline node_t_& hxhash_table<node_t_, table_size_bits_, deleter_t_>::insert_unique(
	const typename node_t_::key_t& key_,
	hxsystem_allocator_t allocator_,
	hxalignment_t alignment_)
{
	node_t_** pos_ = this->get_bucket_head_(hxkey_hash(key_));
	for(node_t_* n_ = *pos_; n_; n_ = (node_t_*)n_->hash_next()) {
		if(hxkey_equal(n_->key(), key_)) {
			return *n_;
		}
	}
	node_t_* n_ = ::new(hxmalloc_ext(sizeof(node_t_), allocator_, alignment_))node_t_(key_);
	n_->hash_next() = *pos_;
	*pos_ = n_;
	++m_size_;
	return *n_;
}

template<hxhash_table_concept_ node_t_, hxhash_t table_size_bits_, typename deleter_t_>
inline void hxhash_table<node_t_, table_size_bits_, deleter_t_>::insert_node(node_t_* ptr_)
{
	hxassertmsg(this->find(ptr_->key()) != ptr_, "container_reinsert");
	hxhash_t hash_ = ptr_->hash();
	node_t_** pos_ = this->get_bucket_head_(hash_);
	ptr_->hash_next() = *pos_;
	*pos_ = ptr_;
	++m_size_;
}

template<hxhash_table_concept_ node_t_, hxhash_t table_size_bits_, typename deleter_t_>
inline node_t_* hxhash_table<node_t_, table_size_bits_, deleter_t_>::find(
	const typename node_t_::key_t& key_, const node_t_* previous_)
{
	if(!previous_) {
		for(node_t_* n_ = *this->get_bucket_head_(hxkey_hash(key_)); n_; n_ = (node_t_*)n_->hash_next()) {
			if(hxkey_equal(n_->key(), key_)) {
				return n_;
			}
		}
	}
	else {
		hxassertmsg(hxkey_equal(key_, previous_->key()), "previous_mismatch");
		hxassertmsg(hxkey_hash(key_) == previous_->hash(), "previous_mismatch");
		for(node_t_* n_ = (node_t_*)previous_->hash_next(); n_; n_ = (node_t_*)n_->hash_next()) {
			if(hxkey_equal(n_->key(), key_)) {
				return n_;
			}
		}
	}
	return hxnull;
}

template<hxhash_table_concept_ node_t_, hxhash_t table_size_bits_, typename deleter_t_>
inline size_t hxhash_table<node_t_, table_size_bits_, deleter_t_>::count(
	const typename node_t_::key_t& key_) const
{
	size_t total_ = 0u;
	hxhash_t hash_ = hxkey_hash(key_);
	for(const node_t_* n_ = *this->get_bucket_head_(hash_); n_; n_ = (const node_t_*)n_->hash_next()) {
		if(hxkey_equal(n_->key(), key_)) {
			++total_;
		}
	}
	return total_;
}

template<hxhash_table_concept_ node_t_, hxhash_t table_size_bits_, typename deleter_t_>
inline node_t_* hxhash_table<node_t_, table_size_bits_, deleter_t_>::extract(
	const typename node_t_::key_t& key_)
{
	hxhash_t hash_ = hxkey_hash(key_);
	node_t_** current_ = this->get_bucket_head_(hash_);
	while(node_t_* n_ = *current_) {
		if(hxkey_equal(n_->key(), key_)) {
			*current_ = (node_t_*)n_->hash_next();
			--m_size_;
			return n_;
		}
		// This avoids special case code for the head pointer.
		current_ = (node_t_**)&n_->hash_next();
	}
	return hxnull;
}

template<hxhash_table_concept_ node_t_, hxhash_t table_size_bits_, typename deleter_t_>
template<typename deleter_override_t_>
inline size_t hxhash_table<node_t_, table_size_bits_, deleter_t_>::erase(
	const typename node_t_::key_t& key_, const deleter_override_t_& deleter_)
{
	size_t count_ = 0u;
	hxhash_t hash_ = hxkey_hash(key_);
	node_t_** current_ = this->get_bucket_head_(hash_);
	while(node_t_* n_ = *current_) {
		if(hxkey_equal(n_->key(), key_)) {
			*current_ = (node_t_*)n_->hash_next();
			if(deleter_) {
				deleter_(n_);
			}
			++count_;
		}
		else {
			current_ = (node_t_**)&n_->hash_next();
		}
	}
	m_size_ -= count_;
	return count_;
}

template<hxhash_table_concept_ node_t_, hxhash_t table_size_bits_, typename deleter_t_>
template<typename deleter_override_t_>
inline void hxhash_table<node_t_, table_size_bits_, deleter_t_>::clear(
	const deleter_override_t_& deleter_)
{
	if(m_size_ != 0u) {
		if(deleter_) {
			node_t_** it_end_ = m_table_.data() + m_table_.capacity();
			for(node_t_** it_ = m_table_.data(); it_ != it_end_; ++it_) {
				node_t_* n_ = *it_;
				if(n_) {
					*it_ = hxnull;
					for(node_t_* t_ = n_; t_; t_ = n_) {
						n_ = (node_t_*)n_->hash_next();
						deleter_(t_);
					}
				}
			}
			m_size_ = 0u;
		}
		else {
			this->release_all();
		}
	}
}

template<hxhash_table_concept_ node_t_, hxhash_t table_size_bits_, typename deleter_t_>
inline void hxhash_table<node_t_, table_size_bits_, deleter_t_>::release_all(void)
{
	if(m_size_ != 0u) {
		::memset(m_table_.data(), 0x00, sizeof(node_t*) * m_table_.capacity());
		m_size_ = 0u;
	}
}

template<hxhash_table_concept_ node_t_, hxhash_t table_size_bits_, typename deleter_t_>
inline size_t hxhash_table<node_t_, table_size_bits_, deleter_t_>::load_max(void) const
{
	size_t maximum_ = 0u;
	const node_t_*const* it_end_ = m_table_.data() + m_table_.capacity();
	for(const node_t_*const* it_ = m_table_.data(); it_ != it_end_; ++it_) {
		size_t count_ = 0u;
		for(const node_t_* n_ = *it_; n_; n_ = (const node_t_*)n_->hash_next()) {
			++count_;
		}
		maximum_ = hxmax(maximum_, count_);
	}
	return maximum_;
}

template<hxhash_table_concept_ node_t_, hxhash_t table_size_bits_, typename deleter_t_>
inline node_t_** hxhash_table<node_t_, table_size_bits_, deleter_t_>::get_bucket_head_(hxhash_t hash_)
{
	hxhash_t index_ = hash_ >> (hxhash_bits - m_table_.get_table_size_bits());
	hxassertmsg(index_ < m_table_.capacity(), "internal_error");
	return m_table_.data() + index_;
}

template<hxhash_table_concept_ node_t_, hxhash_t table_size_bits_, typename deleter_t_>
inline const node_t_*const* hxhash_table<node_t_, table_size_bits_, deleter_t_>::get_bucket_head_(hxhash_t hash_) const
{
	hxhash_t index_ = hash_ >> (hxhash_bits - m_table_.get_table_size_bits());
	hxassertmsg(index_ < m_table_.capacity(), "internal_error");
	return m_table_.data() + index_;
}
