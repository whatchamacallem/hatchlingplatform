#pragma once
// Copyright 2017-2019 Adrian Johnston

#include <hx/internal/hxHashTableInternal.h>

// hxHashTable.h - This header implements a hash table that operates without 
// allocating memory or copying around data.  It can be used as either a map
// or a set and has operations that allow for unique or duplicate keys.  While
// this interface is designed to be familiar, changes will be required to
// switch over code using standard containers.  In particular, all modification
// of the table is non-standard.

// ----------------------------------------------------------------------------
// hxHashTableNodeBase - Base class for hash table entries that are inserted
// into an hxHashTable.  See hxHashTableNodes.h for examples.  Copying and
// modification are disallowed to protect the integrity of the hash table.
//
// A Node (a key or a key-value pair) must subclass hxHashTableNodeBase and
// implement:
//
//   // Construct a new Node from a Key and associated hash value.
//   Node(const Key& key, uint32_t hash);
//
//   // Calculate or return stored hash value for the key.
//   uint32_t hash() const;
//
//   // Calculate hash value for the key.
//   static uint32_t hash(const Key& key) const;
//
//   // Compare a node with a key and its associated hash value.
//   static bool keyEqual(const Node& lhs, const Key& rhs, uint32_t rhsHash) const;
//
// This interface is used to avoid the recalculation of hashes.  keyEqual is
// intended to allow but not require comparison with the hash.  Integer and
// string implementations of this interface are provided.  

template<typename Key_>
class hxHashTableNodeBase {
public:
	typedef Key_ Key;

	// Base class allows hash value to either be stored or recalculated.
	HX_INLINE hxHashTableNodeBase(const Key& k_) : key(k_), m_next(hxnull) { }

	// The key identifies the Node.
	const Key key;

private:
	// key and m_next should not be modified, and m_next should not be copied.
	hxHashTableNodeBase(const hxHashTableNodeBase&); // = delete
	void operator=(const hxHashTableNodeBase&); // = delete

	// The hash table uses m_next to implement an embedded linked list.
	template<typename N_, uint32_t HashBits_> friend class hxHashTable;
	hxHashTableNodeBase* m_next;
};

// ----------------------------------------------------------------------------
// hxHashTable - See top of this file for description.
//
// Node must be a subclass of hxHashTableNode with the interface described above.
// If non-zero HashBits configures the size of the hash table to be HashBits^2.
// Otherwise use set_hash_bits() to configure hash bits dynamically.

template<typename Node_, uint32_t HashBits_=hxAllocatorDynamicCapacity>
class hxHashTable {
public:
	typedef Node_ Node;
	typedef typename Node::Key Key;
	typedef uint32_t size_type;
	enum {
		HashBits = HashBits_,
		HashSize = 1u << HashBits
	};

	// A forward iterator.  Iteration is O(n + (1 << HashBits)).  Iterators are
	// only invalidated by the removal of the Node referenced.  Does not support
	// std::iterator_traits or std::forward_iterator_tag. 
	class const_iterator
	{
	public:
		// Used to implement begin().  (table will not be modfied.)
		HX_INLINE const_iterator(const hxHashTable* table_)
			: m_hashTable(const_cast<hxHashTable*>(table_)), m_nextIndex(0u), m_currentNode(hxnull) { nextBucket(); }

		// Used to implement end().
		HX_INLINE const_iterator() : m_hashTable(hxnull), m_nextIndex(0u), m_currentNode(hxnull) { } // end

		// Standard interface.
		HX_INLINE const_iterator& operator++() {
			hxAssertMsg(m_currentNode, "iterator invalid"); // !end
			if (!(m_currentNode = (Node*)m_currentNode->m_next)) {
				nextBucket();
			}
			return *this;
		}

		// Standard interface.
		HX_INLINE const_iterator operator++(int) { const_iterator t_(*this); operator++(); return t_; }
		HX_INLINE bool operator==(const const_iterator& rhs_) const { return m_currentNode == rhs_.m_currentNode; }
		HX_INLINE bool operator!=(const const_iterator& rhs_) const { return m_currentNode != rhs_.m_currentNode; }
		HX_INLINE const Node& operator*() const { return *m_currentNode; }
		HX_INLINE const Node* operator->() const { return m_currentNode; }

	protected:
		HX_INLINE void nextBucket() {
			hxAssert(m_hashTable && !m_currentNode);
			while (m_nextIndex < m_hashTable->m_table.getCapacity()) {
				if (Node* n_ = m_hashTable->m_table.getStorage()[m_nextIndex++]) {
					m_currentNode = n_;
					return;
				}
			}
		}

		hxHashTable* m_hashTable;
		uint32_t m_nextIndex;
		Node* m_currentNode;
	};

	class iterator : public const_iterator
	{
	public:
		// Used to implement begin().
		HX_INLINE iterator(hxHashTable* tbl_) : const_iterator(tbl_) { }

		// Used to implement end().
		HX_INLINE iterator() { }

		// Standard interface.
		HX_INLINE iterator& operator++() { const_iterator::operator++(); return *this; }
		HX_INLINE iterator operator++(int) { iterator t_(*this); const_iterator::operator++(); return t_; }
		HX_INLINE Node& operator*() const { return *this->m_currentNode; }
		HX_INLINE Node* operator->() const { return this->m_currentNode; }
	};

	// Constructs an empty hash table with a capacity of Capacity.
	HX_INLINE explicit hxHashTable() { m_size = 0u; }

	// Destructs hash table.
	HX_INLINE ~hxHashTable() { clear(); }

	// Standard interface.
	HX_INLINE const_iterator begin() const { return const_iterator(this); }
	HX_INLINE iterator begin() { return iterator(this); }
	HX_INLINE const_iterator cbegin() const { return const_iterator(this); }
	HX_INLINE const_iterator end() const { return const_iterator(); }
	HX_INLINE iterator end() { return iterator(); }
	HX_INLINE const_iterator cend() const { return const_iterator(); }
	HX_INLINE uint32_t size() const { return m_size; }
	HX_INLINE bool empty() const { return m_size != 0u; }

	// Returns Node& for key.  Any allocation required use hxMemoryManagerId_Current
	// and HX_ALIGNMENT_MASK.
	HX_INLINE Node& operator[](const Key& key_) { return insert_unique(key_); }

	// Returns a node containing key if any or allocates and returns a new one.
	HX_INLINE Node& insert_unique(const Key& key_,
								  hxMemoryManagerId id_=hxMemoryManagerId_Current,
								  uintptr_t alignmentMask_=HX_ALIGNMENT_MASK) {
		uint32_t hash_ = Node::hash(key_);
		Node** pos_ = getBucket_(hash_);
		for (Node* n_ = *pos_; n_; n_ = (Node*)n_->m_next) {
			if (Node::keyEqual(*n_, key_, hash_)) {
				return *n_;
			}
		}
		Node* n_ = ::new(hxMallocExt(sizeof(Node), id_, alignmentMask_))Node(key_, hash_);
		n_->m_next = *pos_;
		*pos_ = n_;
		++m_size;
		return *n_;
	}

	// Inserts a node.  Allows multiple nodes of the same Key.
	HX_INLINE void insert_node(Node* node_) {
		hxAssert(node_ != hxnull);
		uint32_t hash_ = node_->hash();
		Node** pos_ = getBucket_(hash_);
		node_->m_next = *pos_;
		*pos_ = node_;
		++m_size;
	}

	// Returns a Node matching key if any.  If previous is non-null it must be
	// a node previously returned from find() with the same key and that has not
	// been removed.  Then find() will return a subsequent node if any.
	HX_INLINE Node* find(const Key& key_, const Node* previous_=hxnull) {
		if (!previous_) {
			uint32_t hash_ = Node::hash(key_);
			for (Node* n_ = *getBucket_(hash_); n_; n_ = (Node*)n_->m_next) {
				if (Node::keyEqual(*n_, key_, hash_)) {
					return n_;
				}
			}
		}
		else {
			hxAssert(Node::keyEqual(*previous_, key_, Node::hash(key_)));
			uint32_t hash_ = previous_->hash();
			for (Node* n_ = (Node*)previous_->m_next; n_; n_ = (Node*)n_->m_next) {
				if (Node::keyEqual(*n_, key_, hash_)) {
					return n_;
				}
			}
		}
		return hxnull;
	}

	// See description of non-const version.
	HX_INLINE const Node* find(const Key& key_, const Node* previous_=hxnull) const {
		// This code calls the non-const version for brevity.
		return const_cast<hxHashTable*>(this)->find(key_, previous_);
	}

	// Returns number of nodes with an equivalent key.
	HX_INLINE uint32_t count(const Key& key_) const {
		uint32_t total_ = 0u;
		uint32_t hash_ = Node::hash(key_);
		for (const Node* n_ = *getBucket_(hash_); n_; n_ = (Node*)n_->m_next) {
			if (Node::keyEqual(*n_, key_, hash_)) {
				++total_;
			}
		}
		return total_;
	}

	// Removes and returns first node with key if any.
	HX_INLINE Node* extract(const Key& key_) {
		uint32_t hash_ = Node::hash(key_);
		Node** next_ = getBucket_(hash_);
		while (Node* n_ = *next_) {
			if (Node::keyEqual(*n_, key_, hash_)) {
				*next_ = (Node*)n_->m_next;
				--m_size;
				return n_;
			}
			next_ = (Node**)&n_->m_next;
		}
		return hxnull;
	}

	// Releases all Nodes matching key and calls deleter() on every node.  Returns
	// the number of nodes released.  Deleter can be functions with signature "void
	// deleter(Node*)" and functors supporting "operator()(Node*)" and with an
	// "operator bool" returning true.
	template<typename Deleter>
	HX_INLINE uint32_t erase(const Key& key_, const Deleter& deleter_) {
		uint32_t count_ = 0u;
		uint32_t hash_ = Node::hash(key_);
		Node** next_ = getBucket_(hash_);
		while (Node* n_ = *next_) {
			if (Node::keyEqual(*n_, key_, hash_)) {
				*next_ = (Node*)n_->m_next;
				if (deleter_) {
					deleter_(n_);
				}
				++count_;
			}
			else {
				next_ = (Node**)&n_->m_next;
			}
		}
		m_size -= count_;
		return count_;
	}

	// Removes and calls hxDelete() on nodes with an equivalent key.
	HX_INLINE uint32_t erase(const Key& key_) { return erase(key_, hxDeleter()); }

	// Removes but does not delete nodes with an equivalent key.
	HX_INLINE uint32_t release_key(const Key& key_) { return erase(key_, (void(*)(Node*))0); }

	// Removes all nodes and calls deleter() on every node.  Deleter can be
	// function pointers with signature "void deleter(Node*)" or functors
	// supporting "operator()(Node*) and operator (bool)."
	template<typename Deleter>
	HX_INLINE void clear(const Deleter& deleter_) {
		if (deleter_) {
			if (m_size != 0u) {
				Node** itEnd_ = m_table.getStorage() + m_table.getCapacity();
				for (Node** it_ = m_table.getStorage(); it_ != itEnd_; ++it_) {
					if (Node* n_ = *it_) {
						*it_ = 0;
						while (Node* t_ = n_) {
							n_ = (Node*)n_->m_next;
							deleter_(t_);
						}
					}
				}
				m_size = 0u;
			}
		}
		else {
			release_all();
		}
	}

	// Removes all nodes and calls hxDelete() on every node.
	HX_INLINE void clear() { clear(hxDeleter()); }

	// Removes but does not delete all nodes.
	HX_INLINE void release_all() {
		if (m_size != 0u) {
			::memset(m_table.getStorage(), 0x00, sizeof(Node*) * m_table.getCapacity());
			m_size = 0u;
		}
	}

	// Returns the number of buckets in the hash table.
	HX_INLINE uint32_t bucket_count() const { return m_table.getCapacity(); };

	// Sets bucket count to be 1 << bits.  Only for use with hxAllocatorDynamicCapacity.
	HX_INLINE void set_hash_bits(uint32_t bits_) { return m_table.setHashBits(bits_); };

	// Returns the average number of nodes per-hash table bucket.
	HX_INLINE float load_factor() const { return (float)m_size / (float)bucket_count(); }

	// Returns size of largest bucket.
	uint32_t load_max() const {
		// An unallocated table will be ok.
		uint32_t maximum_=0u;
		const Node*const* itEnd_ = m_table.getStorage() + m_table.getCapacity();
		for (const Node*const* it_ = m_table.getStorage(); it_ != itEnd_; ++it_) {
			uint32_t count_=0u;
			for (const Node* n_ = *it_; n_; n_ = (const Node*)n_->m_next) {
				++count_;
			}
			maximum_ = hxMax(maximum_, count_);
		}
		return maximum_;
	}

private:
	HX_STATIC_ASSERT(HashBits <= 31u, "hxHashTable: hash bits must be [0..31]");

	hxHashTable(const hxHashTable&); // = delete.  Disables copy and assign.
	void operator=(const hxHashTable&); // = delete

	// Pointer to head of singly-linked list for key's hash value.
	HX_INLINE Node** getBucket_(uint32_t hash_) {
		uint32_t index_ = hash_ >> (32u - m_table.getHashBits());
		hxAssert(index_ < m_table.getCapacity());
		return m_table.getStorage() + index_;
	}

	HX_INLINE const Node*const* getBucket_(uint32_t hash_) const {
		uint32_t index_ = hash_ >> (32u - m_table.getHashBits());
		hxAssert(index_ < m_table.getCapacity());
		return m_table.getStorage() + index_;
	}

	uint32_t m_size;
	hxHashTableInternalAllocator<Node, HashBits> m_table;
};
