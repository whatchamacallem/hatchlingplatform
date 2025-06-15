#pragma once

#include <hx/internal/hxHashTableInternal.hpp>
#include <hx/hxKey.hpp>

// hxHashTable API - This header implements a hash table that operates without
// reallocating memory or copying around data. Each bucket is implemented using
// an embedded linked list. Hash tables can be used as either an unordered map or
// an unordered set and have operations that allow for unique or duplicate keys.
// While this interface is designed to be familiar, changes will be required to
// switch over code using standard containers. In particular, all modification of
// the table is non-standard.
//
// Note that any node T using key K will work as long as it has the following
// fields and K has an operator== or an hxKeyEqual overload.
//
// struct T {
//   typedef K Key;          // tell the hash table what key to use.
//   T(Key);                 // construct from key. e.g. for operator[].
//   void*& hashNext();      // used by hxHashTable for embedded linked list.
//   void* hashNext() const; // const version of hashNext.
//   const Key& key() const; // returns key constructed with.
//   uint32_t hash() const;  // returns hash of key constructed with.
// };
//

// hxHashTableSetNode - Base class for unordered set entries. Caches the hash
// value. Copying and modification are disallowed to protect the integrity of the
// hash table. See hxHashTableMapNode if you need a mutable node.
template<typename Key_>
class hxHashTableSetNode {
public:
	typedef Key_ Key;

	HX_CONSTEXPR_FN hxHashTableSetNode(const Key_& key_)
		: m_hashNext_(hxnull), m_key_(key_)
	{
		// NOTE: You need to implement hxKeyHash for your Key_ type.
		m_hash_ = hxKeyHash(key_);
	}

#if HX_CPLUSPLUS >= 201103L
	HX_CONSTEXPR_FN hxHashTableSetNode(Key_&& key_)
		: m_hashNext_(hxnull), m_key_(key_)
	{
		// NOTE: You need to implement hxKeyHash for your Key_ type.
		m_hash_ = hxKeyHash(m_key_);
	}
#endif

	// Boilerplate required by hxHashTable.
	void* hashNext(void) const { return m_hashNext_; }
	void*& hashNext(void) { return m_hashNext_; }

	// The key and hash identify the Node and should not change once added.
	HX_CONSTEXPR_FN const Key_& key(void) const { return m_key_; }

	// Hash is not required to be unique
	HX_CONSTEXPR_FN uint32_t hash(void) const { return m_hash_; };

private:
	hxHashTableSetNode(void) HX_DELETE_FN;
	// m_hashNext_ should not be copied.
	hxHashTableSetNode(const hxHashTableSetNode&) HX_DELETE_FN;
	void operator=(const hxHashTableSetNode&) HX_DELETE_FN;

	// The hash table uses m_hashNext_ to implement an embedded linked list.
	void* m_hashNext_;
	Key_ m_key_;
	uint32_t m_hash_;
};

// Base class for unordered map entries.
template<typename Key_, typename Value_>
class hxHashTableMapNode : public hxHashTableSetNode<Key_> {
public:
	typedef Key_ Key;
	typedef Value_ Value;

	// Value_ must default construct when using operator[].
	HX_CONSTEXPR_FN hxHashTableMapNode(const Key_& key_) :
		hxHashTableSetNode<Key_>(key_) { }

	HX_CONSTEXPR_FN hxHashTableMapNode(const Key_& key_, const Value_& value_) :
		hxHashTableSetNode<Key_>(key_), m_value_(value_) { }

#if HX_CPLUSPLUS >= 201103L
	HX_CONSTEXPR_FN hxHashTableMapNode(const Key_& key_, Value_&& value_) :
		hxHashTableSetNode<Key_>(key_), m_value_(value_) { }
#endif

	const Value_& value() const { return m_value_; }
	Value_& value() { return m_value_; }
protected:
	Value_ m_value_;
};

// hxHashTable - See top of this file for description.
//
// Node must be a subclass of hxHashTableNode with the interface described above.
// If non-zero TableSizeBits configures the size of the hash table to be TableSizeBits^2.
// Otherwise use setTableSizeBits() to configure hash bits dynamically.
template<typename Node_, uint32_t TableSizeBits_=hxAllocatorDynamicCapacity>
class hxHashTable {
public:
	typedef Node_ Node;
	typedef typename Node_::Key Key;

	// A forward iterator. Iteration is O(n + (1 << TableSizeBits)).
	// Iterators are only invalidated by the removal of the Node referenced.
	class constIterator
	{
	public:
		// Constructs an iterator pointing to the beginning of the hash table.
		HX_CONSTEXPR_FN constIterator(const hxHashTable* table_)
			: m_hashTable_(const_cast<hxHashTable*>(table_)), m_nextIndex_(0u), m_currentNode(hxnull) { nextBucket(); }

		// Constructs an iterator pointing to the end of the hash table.
		HX_CONSTEXPR_FN constIterator() : m_hashTable_(hxnull), m_nextIndex_(0u), m_currentNode(hxnull) { } // end

		// Advances the iterator to the next element.
		HX_CONSTEXPR_FN constIterator& operator++() {
			hxAssertMsg(m_currentNode, "iterator invalid"); // !end
			if (!(m_currentNode = (Node_*)m_currentNode->hashNext())) {
				nextBucket();
			}
			return *this;
		}

		// Advances the iterator to the next element (post-increment).
		HX_CONSTEXPR_FN constIterator operator++(int) { constIterator t_(*this); operator++(); return t_; }

		// Compares two iterators for equality.
		HX_CONSTEXPR_FN bool operator==(const constIterator& rhs_) const { return m_currentNode == rhs_.m_currentNode; }

		// Compares two iterators for inequality.
		HX_CONSTEXPR_FN bool operator!=(const constIterator& rhs_) const { return m_currentNode != rhs_.m_currentNode; }

		// Dereferences the iterator to access the current Node.
		HX_CONSTEXPR_FN const Node_& operator*() const { return *m_currentNode; }

		// Dereferences the iterator to access the current Node's pointer.
		HX_CONSTEXPR_FN const Node_* operator->() const { return m_currentNode; }

	protected:
		// Advances the iterator to the next non-empty bucket.
		HX_CONSTEXPR_FN void nextBucket() {
			hxAssert(m_hashTable_ && !m_currentNode);
			while (m_nextIndex_ < m_hashTable_->m_table.capacity()) {
				if (Node_* n_ = m_hashTable_->m_table.data()[m_nextIndex_++]) {
					m_currentNode = n_;
					return;
				}
			}
		}

		hxHashTable* m_hashTable_;
		uint32_t m_nextIndex_;
		Node_* m_currentNode;
	};

	// A mutable iterator that can modify the elements of the hash table.
	class iterator : public constIterator
	{
	public:
		// Constructs an iterator pointing to the beginning of the hash table.
		HX_CONSTEXPR_FN iterator(hxHashTable* tbl_) : constIterator(tbl_) { }

		// Constructs an iterator pointing to the end of the hash table.
		HX_CONSTEXPR_FN iterator() { }

		// Advances the iterator to the next element.
		HX_CONSTEXPR_FN iterator& operator++() { constIterator::operator++(); return *this; }

		// Advances the iterator to the next element (post-increment).
		HX_CONSTEXPR_FN iterator operator++(int) { iterator cit_(*this); constIterator::operator++(); return cit_; }

		// Dereferences the iterator to access the current Node.
		HX_CONSTEXPR_FN Node_& operator*() const { return *this->m_currentNode; }

		// Dereferences the iterator to access the current Node's pointer.
		HX_CONSTEXPR_FN Node_* operator->() const { return this->m_currentNode; }
	};

	// Constructs an empty hash table with a capacity of TableSizeBits^2.
	HX_CONSTEXPR_FN explicit hxHashTable() { m_size_ = 0u; }

	// Destructs the hash table and releases all resources.
#if HX_CPLUSPLUS >= 202002L
	constexpr
#endif
	~hxHashTable() { clear(); }

	// Returns a const iterator pointing to the beginning of the hash table.
	HX_CONSTEXPR_FN constIterator begin() const { return constIterator(this); }

	// Returns an iterator pointing to the beginning of the hash table.
	HX_CONSTEXPR_FN iterator begin() { return iterator(this); }

	// Returns a const iterator pointing to the beginning of the hash table.
	HX_CONSTEXPR_FN constIterator cBegin() const { return constIterator(this); }

	// Returns a const iterator pointing to the beginning of the hash table.
	HX_CONSTEXPR_FN constIterator cBegin() { return constIterator(this); }

	// Returns a const iterator pointing to the end of the hash table.
	HX_CONSTEXPR_FN constIterator end() const { return constIterator(); }

	// Returns an iterator pointing to the end of the hash table.
	HX_CONSTEXPR_FN iterator end() { return iterator(); }

	// Returns a const iterator pointing to the end of the hash table.
	HX_CONSTEXPR_FN constIterator cEnd() const { return constIterator(); }

	// Returns a const iterator pointing to the end of the hash table.
	HX_CONSTEXPR_FN constIterator cEnd() { return constIterator(); }

	// Returns the number of elements in the hash table.
	HX_CONSTEXPR_FN uint32_t size() const { return m_size_; }

	// Checks if the hash table is empty.
	HX_CONSTEXPR_FN bool empty() const { return m_size_ == 0u; }

	// Returns a node containing key if any or allocates and returns a new one.
	// Any allocation required uses hxMemoryAllocator_Current and HX_ALIGNMENT.
	// - key: The key to search for or insert.
	HX_CONSTEXPR_FN Node_& operator[](const typename Node_::Key& key_) { return this->insertUnique(key_); }

	// Returns a node containing key if any or allocates and returns a new one.
	// Unfortunately this code may calculate the hash twice.
	// - key: The key to search for or insert.
	// - allocator: The memory manager ID to use for allocation. Defaults to hxMemoryAllocator_Current.
	// - alignment: The alignment for allocation. Defaults to HX_ALIGNMENT.
	HX_CONSTEXPR_FN Node_& insertUnique(const typename Node_::Key& key_,
										hxMemoryAllocator allocator_=hxMemoryAllocator_Current,
										uintptr_t alignment_=HX_ALIGNMENT) {
		Node_** pos_ = this->getBucketHead_(hxKeyHash(key_));
		for (Node_* n_ = *pos_; n_; n_ = (Node_*)n_->hashNext()) {
			if (hxKeyEqual(n_->key(), key_)) {
				return *n_;
			}
		}
		hxAssert(m_size_ < ~(uint32_t)0);
		Node_* n_ = ::new(hxMallocExt(sizeof(Node_), allocator_, alignment_))Node_(key_);
		n_->hashNext() = *pos_;
		*pos_ = n_;
		++m_size_;
		return *n_;
	}

	// Inserts a Node into the hash table, allowing duplicate keys. Nodes that
	// have non-null hash pointers are allowed because they may have been released
	// from a table that way.
	// - node: The Node to insert into the hash table.
	HX_CONSTEXPR_FN void insertNode(Node_* node_) {
		hxAssert(node_ != hxnull && m_size_ < ~(uint32_t)0);
		hxAssert(this->find(node_->key()) != node_);
		uint32_t hash_ = node_->hash();
		Node_** pos_ = this->getBucketHead_(hash_);
		node_->hashNext() = *pos_;
		*pos_ = node_;
		++m_size_;
	}

	// Returns a Node matching key if any. If previous is non-null it must be
	// a node previously returned from find() with the same key and that has not
	// been removed. Then find() will return a subsequent node if any.
	// The previous object is non-const as it may be modified.
	// - key: The key to search for in the hash table.
	// - previous: A previously found Node with the same key, or nullptr.
	HX_CONSTEXPR_FN Node_* find(const typename Node_::Key& key_, const Node_* previous_=hxnull) {
		if (!previous_) {
			for (Node_* n_ = *this->getBucketHead_(hxKeyHash(key_)); n_; n_ = (Node_*)n_->hashNext()) {
				if (hxKeyEqual(n_->key(), key_)) {
					return n_;
				}
			}
		}
		else {
			hxAssert(hxKeyEqual(key_, previous_->key()));
			hxAssert(hxKeyHash(key_) == previous_->hash());
			for (Node_* n_ = (Node_*)previous_->hashNext(); n_; n_ = (Node_*)n_->hashNext()) {
				if (hxKeyEqual(n_->key(), key_)) {
					return n_;
				}
			}
		}
		return hxnull;
	}

	// Const version.
	HX_CONSTEXPR_FN const Node_* find(const typename Node_::Key& key_, const Node_* previous_=hxnull) const {
		// This code calls the non-const version for brevity.
		return const_cast<hxHashTable*>(this)->find(key_, previous_);
	}

	// Counts the number of Nodes with the given key.
	// - key: The key to count occurrences of in the hash table.
	HX_CONSTEXPR_FN uint32_t count(const typename Node_::Key& key_) const {
		uint32_t total_ = 0u;
		uint32_t hash_ = hxKeyHash(key_);
		for (const Node_* n_ = *this->getBucketHead_(hash_); n_; n_ = (Node_*)n_->hashNext()) {
			if (hxKeyEqual(n_->key(), key_)) {
				++total_;
			}
		}
		return total_;
	}

	// Removes and returns the first Node with the given key.
	// - key: The key to search for and remove from the hash table.
	HX_CONSTEXPR_FN Node_* extract(const typename Node_::Key& key_) {
		uint32_t hash_ = hxKeyHash(key_);
		Node_** current_ = this->getBucketHead_(hash_);
		while (Node_* n_ = *current_) {
			if (hxKeyEqual(n_->key(), key_)) {
				*current_ = (Node_*)n_->hashNext();
				--m_size_;
				return n_;
			}
			// This avoids special case code for the head pointer.
			current_ = (Node_**)&n_->hashNext();
		}
		return hxnull;
	}

	// Releases all Nodes matching key and calls deleter() on every node. Returns
	// the number of nodes released. Deleter can be functions with signature "void
	// deleter(Node*)" and functors supporting "operator()(Node*)" and with an
	// "operator bool" returning true.
	// - key: The key to search for and remove from the hash table.
	// - deleter: A function or functor to call on each removed Node.
	template<typename Deleter_>
	HX_CONSTEXPR_FN uint32_t erase(const typename Node_::Key& key_, const Deleter_& deleter_) {
		uint32_t count_ = 0u;
		uint32_t hash_ = hxKeyHash(key_);
		Node_** current_ = this->getBucketHead_(hash_);
		while (Node_* n_ = *current_) {
			if (hxKeyEqual(n_->key(), key_)) {
				*current_ = (Node_*)n_->hashNext();
				if (deleter_) {
					deleter_(n_);
				}
				++count_;
			}
			else {
				current_ = (Node_**)&n_->hashNext();
			}
		}
		m_size_ -= count_;
		return count_;
	}

	// Removes and calls hxDelete() on nodes with an equivalent key.
	HX_CONSTEXPR_FN uint32_t erase(const typename Node_::Key& key_) { return erase(key_, hxDeleter()); }

	// Removes all Nodes matching the given key without deleting them.
	HX_CONSTEXPR_FN uint32_t releaseKey(const typename Node_::Key& key_) { return erase(key_, (void(*)(Node_*))0); }

	// Removes all nodes and calls deleter() on every node. Deleter can be
	// function pointers with signature "void deleter(Node*)" or functors
	// supporting "operator()(Node*) and operator (bool)."
	// - deleter: A function or functor to call on each removed Node.
	template<typename Deleter_>
	HX_CONSTEXPR_FN void clear(const Deleter_& deleter_) {
		if (m_size_ != 0u) {
			if (deleter_) {
				Node_** itEnd_ = m_table.data() + m_table.capacity();
				for (Node_** it_ = m_table.data(); it_ != itEnd_; ++it_) {
					Node_* n_ = *it_;
					while (Node_* t_ = n_) {
						n_ = (Node_*)n_->hashNext();
						deleter_(t_);
					}
				}
			}
			// zero the table either way.
			this->releaseAll();
		}
	}

	// Removes all nodes and calls hxDelete() on every node.
	HX_CONSTEXPR_FN void clear() { this->clear(hxDeleter()); }

	// Clears the hash table without deleting any Nodes.
	HX_CONSTEXPR_FN void releaseAll() {
		if (m_size_ != 0u) {
			::memset(m_table.data(), 0x00, sizeof(Node*) * m_table.capacity());
			m_size_ = 0u;
		}
	}

	// Returns the number of buckets in the hash table.
	HX_CONSTEXPR_FN uint32_t bucketCount() const { return m_table.capacity(); };

	// Sets the number of hash bits (only for dynamic capacity).
	// - bits: The number of hash bits to set for the hash table.
	HX_CONSTEXPR_FN void setTableSizeBits(uint32_t bits_) { return m_table.setTableSizeBits(bits_); };

	// Returns the average number of Nodes per bucket.
	HX_CONSTEXPR_FN float loadFactor() const { return (float)m_size_ / (float)this->bucketCount(); }

	// Returns the size of the largest bucket.
	uint32_t loadMax() const {
		// An unallocated table will be ok.
		uint32_t maximum_=0u;
		const Node_*const* itEnd_ = m_table.data() + m_table.capacity();
		for (const Node_*const* it_ = m_table.data(); it_ != itEnd_; ++it_) {
			uint32_t count_=0u;
			for (const Node_* n_ = *it_; n_; n_ = (const Node_*)n_->hashNext()) {
				++count_;
			}
			maximum_ = hxmax(maximum_, count_);
		}
		return maximum_;
	}

private:
	HX_STATIC_ASSERT(TableSizeBits_ <= 31u, "hxHashTable: hash bits must be [0..31]");

	// Not ideal.
    hxHashTable(const hxHashTable&) HX_DELETE_FN;

	// Pointer to head of singly-linked list for key's hash value.
	HX_CONSTEXPR_FN Node_** getBucketHead_(uint32_t hash_) {
		uint32_t index_ = hash_ >> (32u - m_table.getTableSizeBits());
		hxAssert(index_ < m_table.capacity());
		return m_table.data() + index_;
	}

	HX_CONSTEXPR_FN const Node_*const* getBucketHead_(uint32_t hash_) const {
		uint32_t index_ = hash_ >> (32u - m_table.getTableSizeBits());
		hxAssert(index_ < m_table.capacity());
		return m_table.data() + index_;
	}

	uint32_t m_size_;
	hxHashTableInternalAllocator_<Node_, TableSizeBits_> m_table;
};
