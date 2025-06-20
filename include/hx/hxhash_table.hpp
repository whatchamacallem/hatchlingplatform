#pragma once

#include <hx/internal/hxhash_table_internal.hpp>
#include <hx/hxkey.hpp>

// hxhash_table API - This header implements a hash table that operates without
// reallocating memory or copying around data. Each bucket is implemented using
// an embedded linked list. Hash tables can be used as either an unordered map or
// an unordered set and have operations that allow for unique or duplicate keys.
// While this interface is designed to be familiar, changes will be required to
// switch over code using standard containers. In particular, all modification of
// the table is non-standard.
//
// Note that any node T using key K will work as long as it has the following
// fields and K has an operator== or an hxkey_equal overload.
//
// class T {
//   typedef K key_t;          // tell the hash table what key to use.
//   T(key_t);                 // construct from key. e.g. for operator[].
//   void*& hash_next();      // used by hxhash_table for embedded linked list.
//   void* hash_next() const; // const version of hash_next.
//   const key_t& key() const; // returns key constructed with.
//   uint32_t hash() const;  // returns hash of key constructed with.
// };
//

// hxhash_table_set_node - Base class for unordered set entries. Caches the hash
// value. Copying and modification are disallowed to protect the integrity of the
// hash table. See hxhash_table_map_node if you need a mutable node.
template<typename key_t_>
class hxhash_table_set_node {
public:
	typedef key_t_ key_t;

	HX_CONSTEXPR_FN hxhash_table_set_node(const key_t_& key_)
		: m_hash_next_(hxnull), m_key_(key_)
	{
		// NOTE: You need to implement hxkey_hash for your key_t_ type.
		m_hash_ = hxkey_hash(key_);
	}

#if HX_CPLUSPLUS >= 201103L
	HX_CONSTEXPR_FN hxhash_table_set_node(key_t_&& key_)
		: m_hash_next_(hxnull), m_key_(key_)
	{
		// NOTE: You need to implement hxkey_hash for your key_t_ type.
		m_hash_ = hxkey_hash(m_key_);
	}
#endif

	// Boilerplate required by hxhash_table.
	void* hash_next(void) const { return m_hash_next_; }
	void*& hash_next(void) { return m_hash_next_; }

	// The key and hash identify the Node and should not change once added.
	HX_CONSTEXPR_FN const key_t_& key(void) const { return m_key_; }

	// Hash is not required to be unique
	HX_CONSTEXPR_FN uint32_t hash(void) const { return m_hash_; };

private:
	hxhash_table_set_node(void) HX_DELETE_FN;
	// m_hash_next_ should not be copied.
	hxhash_table_set_node(const hxhash_table_set_node&) HX_DELETE_FN;
	void operator=(const hxhash_table_set_node&) HX_DELETE_FN;

	// The hash table uses m_hash_next_ to implement an embedded linked list.
	void* m_hash_next_;
	key_t_ m_key_;
	uint32_t m_hash_;
};

// Base class for unordered map entries.
template<typename key_t_, typename value_t_>
class hxhash_table_map_node : public hxhash_table_set_node<key_t_> {
public:
	typedef key_t_ key_t;
	typedef value_t_ value_t;

	// value_t_ must default construct when using operator[].
	HX_CONSTEXPR_FN hxhash_table_map_node(const key_t_& key_) :
		hxhash_table_set_node<key_t_>(key_) { }

	HX_CONSTEXPR_FN hxhash_table_map_node(const key_t_& key_, const value_t_& value_) :
		hxhash_table_set_node<key_t_>(key_), m_value_(value_) { }

#if HX_CPLUSPLUS >= 201103L
	HX_CONSTEXPR_FN hxhash_table_map_node(const key_t_& key_, value_t_&& value_) :
		hxhash_table_set_node<key_t_>(key_), m_value_(value_) { }
#endif

	const value_t_& value() const { return m_value_; }
	value_t_& value() { return m_value_; }
protected:
	value_t_ m_value_;
};

// hxhash_table - See top of this file for description.
//
// Node must be a subclass of hxhash_table_node with the interface described above.
// If non-zero Table_size_bits configures the size of the hash table to be Table_size_bits^2.
// Otherwise use set_table_size_bits() to configure hash bits dynamically.
// See hxdo_not_delete for situations where the table does not own the nodes.
template<typename node_t_,
	uint32_t table_size_bits_=hxallocator_dynamic_capacity,
	typename deleter_t_=hxdeleter>
class hxhash_table {
public:
	typedef node_t_ Node;
	typedef typename node_t_::key_t key_t;

	// A forward iterator. Iteration is O(n + (1 << Table_size_bits)).
	// Iterators are only invalidated by the removal of the Node referenced.
	class const_iterator
	{
	public:
		// Constructs an iterator pointing to the beginning of the hash table.
		HX_CONSTEXPR_FN const_iterator(const hxhash_table* table_)
			: m_hash_table_(const_cast<hxhash_table*>(table_)), m_next_index_(0u), m_current_node_(hxnull) { next_bucket(); }

		// Constructs an iterator pointing to the end of the hash table.
		HX_CONSTEXPR_FN const_iterator() : m_hash_table_(hxnull), m_next_index_(0u), m_current_node_(hxnull) { } // end

		// Advances the iterator to the next element.
		HX_CONSTEXPR_FN const_iterator& operator++() {
			hxassertmsg(m_current_node_, "iterator invalid"); // !end
			if (!(m_current_node_ = (node_t_*)m_current_node_->hash_next())) {
				next_bucket();
			}
			return *this;
		}

		// Advances the iterator to the next element (post-increment).
		HX_CONSTEXPR_FN const_iterator operator++(int) { const_iterator t_(*this); operator++(); return t_; }

		// Compares two iterators for equality.
		HX_CONSTEXPR_FN bool operator==(const const_iterator& rhs_) const { return m_current_node_ == rhs_.m_current_node_; }

		// Compares two iterators for inequality.
		HX_CONSTEXPR_FN bool operator!=(const const_iterator& rhs_) const { return m_current_node_ != rhs_.m_current_node_; }

		// Dereferences the iterator to access the current Node.
		HX_CONSTEXPR_FN const node_t_& operator*() const { return *m_current_node_; }

		// Dereferences the iterator to access the current Node's pointer.
		HX_CONSTEXPR_FN const node_t_* operator->() const { return m_current_node_; }

	protected:
		// Advances the iterator to the next non-empty bucket.
		HX_CONSTEXPR_FN void next_bucket() {
			hxassert(m_hash_table_ && !m_current_node_);
			while (m_next_index_ < m_hash_table_->m_table_.capacity()) {
				if (node_t_* n_ = m_hash_table_->m_table_.data()[m_next_index_++]) {
					m_current_node_ = n_;
					return;
				}
			}
		}

		hxhash_table* m_hash_table_;
		uint32_t m_next_index_;
		node_t_* m_current_node_;
	};

	// A mutable iterator that can modify the elements of the hash table.
	class iterator : public const_iterator
	{
	public:
		// Constructs an iterator pointing to the beginning of the hash table.
		HX_CONSTEXPR_FN iterator(hxhash_table* tbl_) : const_iterator(tbl_) { }

		// Constructs an iterator pointing to the end of the hash table.
		HX_CONSTEXPR_FN iterator() { }

		// Advances the iterator to the next element.
		HX_CONSTEXPR_FN iterator& operator++() { const_iterator::operator++(); return *this; }

		// Advances the iterator to the next element (post-increment).
		HX_CONSTEXPR_FN iterator operator++(int) { iterator cit_(*this); const_iterator::operator++(); return cit_; }

		// Dereferences the iterator to access the current Node.
		HX_CONSTEXPR_FN node_t_& operator*() const { return *this->m_current_node_; }

		// Dereferences the iterator to access the current Node's pointer.
		HX_CONSTEXPR_FN node_t_* operator->() const { return this->m_current_node_; }
	};

	// Constructs an empty hash table with a capacity of Table_size_bits^2.
	HX_CONSTEXPR_FN explicit hxhash_table() { m_size_ = 0u; }

	// Destructs the hash table and releases all resources.
#if HX_CPLUSPLUS >= 202002L
	constexpr
#endif
	~hxhash_table() { clear(); }

	// Returns a const iterator pointing to the beginning of the hash table.
	HX_CONSTEXPR_FN const_iterator begin() const { return const_iterator(this); }

	// Returns an iterator pointing to the beginning of the hash table.
	HX_CONSTEXPR_FN iterator begin() { return iterator(this); }

	// Returns a const iterator pointing to the beginning of the hash table.
	HX_CONSTEXPR_FN const_iterator c_begin() const { return const_iterator(this); }

	// Returns a const iterator pointing to the beginning of the hash table.
	HX_CONSTEXPR_FN const_iterator c_begin() { return const_iterator(this); }

	// Returns a const iterator pointing to the end of the hash table.
	HX_CONSTEXPR_FN const_iterator end() const { return const_iterator(); }

	// Returns an iterator pointing to the end of the hash table.
	HX_CONSTEXPR_FN iterator end() { return iterator(); }

	// Returns a const iterator pointing to the end of the hash table.
	HX_CONSTEXPR_FN const_iterator c_end() const { return const_iterator(); }

	// Returns a const iterator pointing to the end of the hash table.
	HX_CONSTEXPR_FN const_iterator c_end() { return const_iterator(); }

	// Returns the number of elements in the hash table.
	HX_CONSTEXPR_FN uint32_t size() const { return m_size_; }

	// Checks if the hash table is empty.
	HX_CONSTEXPR_FN bool empty() const { return m_size_ == 0u; }

	// Returns a node containing key if any or allocates and returns a new one.
	// Any allocation required uses hxmemory_allocator_current and HX_ALIGNMENT.
	// - key: The key to search for or insert.
	HX_CONSTEXPR_FN node_t_& operator[](const typename node_t_::key_t& key_) { return this->insert_unique(key_); }

	// Returns a node containing key if any or allocates and returns a new one.
	// Unfortunately this code may calculate the hash twice.
	// - key: The key to search for or insert.
	// - allocator: The memory manager ID to use for allocation. Defaults to hxmemory_allocator_current.
	// - alignment: The alignment for allocation. Defaults to HX_ALIGNMENT.
	HX_CONSTEXPR_FN node_t_& insert_unique(const typename node_t_::key_t& key_,
										hxmemory_allocator allocator_=hxmemory_allocator_current,
										uintptr_t alignment_=HX_ALIGNMENT) {
		node_t_** pos_ = this->get_bucket_head_(hxkey_hash(key_));
		for (node_t_* n_ = *pos_; n_; n_ = (node_t_*)n_->hash_next()) {
			if (hxkey_equal(n_->key(), key_)) {
				return *n_;
			}
		}
		hxassert(m_size_ < ~(uint32_t)0);
		node_t_* n_ = ::new(hxmalloc_ext(sizeof(node_t_), allocator_, alignment_))node_t_(key_);
		n_->hash_next() = *pos_;
		*pos_ = n_;
		++m_size_;
		return *n_;
	}

	// Inserts a Node into the hash table, allowing duplicate keys. Nodes that
	// have non-null hash pointers are allowed because they may have been released
	// from a table that way.
	// - node: The Node to insert into the hash table.
	HX_CONSTEXPR_FN void insert_node(node_t_* ptr_) {
		hxassert(ptr_ != hxnull && m_size_ < ~(uint32_t)0);
		hxassert(this->find(ptr_->key()) != ptr_);
		uint32_t hash_ = ptr_->hash();
		node_t_** pos_ = this->get_bucket_head_(hash_);
		ptr_->hash_next() = *pos_;
		*pos_ = ptr_;
		++m_size_;
	}

	// Returns a Node matching key if any. If previous is non-null it must be
	// a node previously returned from find() with the same key and that has not
	// been removed. Then find() will return a subsequent node if any.
	// The previous object is non-const as it may be modified.
	// - key: The key to search for in the hash table.
	// - previous: A previously found Node with the same key, or nullptr.
	HX_CONSTEXPR_FN node_t_* find(const typename node_t_::key_t& key_, const node_t_* previous_=hxnull) {
		if (!previous_) {
			for (node_t_* n_ = *this->get_bucket_head_(hxkey_hash(key_)); n_; n_ = (node_t_*)n_->hash_next()) {
				if (hxkey_equal(n_->key(), key_)) {
					return n_;
				}
			}
		}
		else {
			hxassert(hxkey_equal(key_, previous_->key()));
			hxassert(hxkey_hash(key_) == previous_->hash());
			for (node_t_* n_ = (node_t_*)previous_->hash_next(); n_; n_ = (node_t_*)n_->hash_next()) {
				if (hxkey_equal(n_->key(), key_)) {
					return n_;
				}
			}
		}
		return hxnull;
	}

	// Const version.
	HX_CONSTEXPR_FN const node_t_* find(const typename node_t_::key_t& key_, const node_t_* previous_=hxnull) const {
		// This code calls the non-const version for brevity.
		return const_cast<hxhash_table*>(this)->find(key_, previous_);
	}

	// Counts the number of Nodes with the given key.
	// - key: The key to count occurrences of in the hash table.
	HX_CONSTEXPR_FN uint32_t count(const typename node_t_::key_t& key_) const {
		uint32_t total_ = 0u;
		uint32_t hash_ = hxkey_hash(key_);
		for (const node_t_* n_ = *this->get_bucket_head_(hash_); n_; n_ = (node_t_*)n_->hash_next()) {
			if (hxkey_equal(n_->key(), key_)) {
				++total_;
			}
		}
		return total_;
	}

	// Removes and returns the first Node with the given key.
	// - key: The key to search for and remove from the hash table.
	HX_CONSTEXPR_FN node_t_* extract(const typename node_t_::key_t& key_) {
		uint32_t hash_ = hxkey_hash(key_);
		node_t_** current_ = this->get_bucket_head_(hash_);
		while (node_t_* n_ = *current_) {
			if (hxkey_equal(n_->key(), key_)) {
				*current_ = (node_t_*)n_->hash_next();
				--m_size_;
				return n_;
			}
			// This avoids special case code for the head pointer.
			current_ = (node_t_**)&n_->hash_next();
		}
		return hxnull;
	}

	// Releases all Nodes matching key and calls deleter() on every node. Returns
	// the number of nodes released. Deleter can be functions with signature "void
	// deleter(Node*)" and functors supporting "operator()(Node*)" and with an
	// "operator bool" returning true. E.g. a free list or a null pointer.
	// - key: The key to search for and remove from the hash table.
	// - deleter: A function or functor to call on each removed Node.
	template<typename Deleter_actual_>
	HX_CONSTEXPR_FN uint32_t erase(const typename node_t_::key_t& key_, const Deleter_actual_& deleter_) {
		uint32_t count_ = 0u;
		uint32_t hash_ = hxkey_hash(key_);
		node_t_** current_ = this->get_bucket_head_(hash_);
		while (node_t_* n_ = *current_) {
			if (hxkey_equal(n_->key(), key_)) {
				*current_ = (node_t_*)n_->hash_next();
				if (deleter_) {
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

	// Removes and calls hxdelete() on nodes with an equivalent key.
	HX_CONSTEXPR_FN uint32_t erase(const typename node_t_::key_t& key_) {
		return this->erase(key_, deleter_t_());
	}

	// Removes all Nodes matching the given key without deleting them.
	HX_CONSTEXPR_FN uint32_t release_key(const typename node_t_::key_t& key_) {
		// Pass a null pointer for the deleter. Just to show off.
		return this->erase(key_, (void(*)(node_t_*))0);
	}

	// Removes all nodes and calls deleter() on every node. Deleter can be
	// function pointers with signature "void deleter(Node*)" or functors
	// supporting "operator()(Node*) and operator (bool)." deleter could be
	// a free list or a null function pointer.
	// - deleter: A function or functor to call on each removed Node.
	template<typename Deleter_actual_>
	HX_CONSTEXPR_FN void clear(const Deleter_actual_& deleter_) {
		if (m_size_ != 0u) {
			if (deleter_) {
				node_t_** it_end_ = m_table_.data() + m_table_.capacity();
				for (node_t_** it_ = m_table_.data(); it_ != it_end_; ++it_) {
					node_t_* n_ = *it_;
					if(n_) {
						*it_ = hxnull;
						for(node_t_* t_ = n_; t_; t_=n_) {
							n_ = (node_t_*)n_->hash_next();
							deleter_(t_);
						}
					}
				}
			}
			else {
				this->release_all();
			}
		}
	}

	// Removes all nodes and calls hxdelete() on every node.
	HX_CONSTEXPR_FN void clear() { this->clear(deleter_t_()); }

	// Clears the hash table without deleting any Nodes.
	HX_CONSTEXPR_FN void release_all() {
		if (m_size_ != 0u) {
			::memset(m_table_.data(), 0x00, sizeof(Node*) * m_table_.capacity());
			m_size_ = 0u;
		}
	}

	// Returns the number of buckets in the hash table.
	HX_CONSTEXPR_FN uint32_t bucket_count() const { return m_table_.capacity(); };

	// Sets the number of hash bits (only for dynamic capacity).
	// - bits: The number of hash bits to set for the hash table.
	HX_CONSTEXPR_FN void set_table_size_bits(uint32_t bits_) { return m_table_.set_table_size_bits(bits_); };

	// Returns the average number of Nodes per bucket.
	HX_CONSTEXPR_FN float load_factor() const { return (float)m_size_ / (float)this->bucket_count(); }

	// Returns the size of the largest bucket.
	uint32_t load_max() const {
		// An unallocated table will be ok.
		uint32_t maximum_=0u;
		const node_t_*const* it_end_ = m_table_.data() + m_table_.capacity();
		for (const node_t_*const* it_ = m_table_.data(); it_ != it_end_; ++it_) {
			uint32_t count_=0u;
			for (const node_t_* n_ = *it_; n_; n_ = (const node_t_*)n_->hash_next()) {
				++count_;
			}
			maximum_ = hxmax(maximum_, count_);
		}
		return maximum_;
	}

private:
	HX_STATIC_ASSERT(table_size_bits_ <= 31u, "hxhash_table: hash bits must be [0..31]");

	// Not ideal.
    hxhash_table(const hxhash_table&) HX_DELETE_FN;

	// Pointer to head of singly-linked list for key's hash value.
	HX_CONSTEXPR_FN node_t_** get_bucket_head_(uint32_t hash_) {
		uint32_t index_ = hash_ >> (32u - m_table_.get_table_size_bits());
		hxassert(index_ < m_table_.capacity());
		return m_table_.data() + index_;
	}

	HX_CONSTEXPR_FN const node_t_*const* get_bucket_head_(uint32_t hash_) const {
		uint32_t index_ = hash_ >> (32u - m_table_.get_table_size_bits());
		hxassert(index_ < m_table_.capacity());
		return m_table_.data() + index_;
	}

	uint32_t m_size_;
	hxhash_table_internal_allocator_<node_t_, table_size_bits_> m_table_;
};
