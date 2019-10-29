
#pragma once
// Copyright 2017 Adrian Johnston

#include "hxHashTableInternal.h"

// hxHashTable - This header implements a hash table that can operate without 
// allocating memory or copying around data.  It can be used as either a map
// or a set and has operations that allow for unique or duplicate keys.  While
// this interface is designed to be familiar, changes will be required to
// switch over code using standard containers.  In particular, all modification
// of the table is non-standard.

template<typename N, uint32_t HashBits>
class hxHashTable;

// ----------------------------------------------------------------------------
// hxHashTableNodeBase - Base class for hash table entries that are inserted
// into an hxHashTable.
//
// A node (a key or a key-value pair) must subclass hxHashTableNodeBase and
// implement:
//
//   Node(const Key& key, uint32_t hash);
//   uint32_t hash() const;
//   static uint32_t hash(const Key& key) const;
//   static bool keyEqual(const Node& lhs, const Key& rhs, uint32_t rhsHash) const;
//
// This interface is used to avoid the recalculation of hashes.  keyEqual is
// intended to allow but not require comparison with the hash.  Integer and
// string implementations of this interface are provided.  

template<typename K>
class hxHashTableNodeBase {
public:
	static const uint32_t c_hashMultiplier = 0x61C88647u; // See the Linux hash.h

	typedef K Key;
	const Key key;

	HX_INLINE hxHashTableNodeBase(const Key& k) : key(k), m_next(hx_null) { }

private:
	template<typename N, uint32_t HashBits>
	friend class hxHashTable;

	hxHashTableNodeBase(const hxHashTableNodeBase&); // = delete
	void operator=(const hxHashTableNodeBase&); // = delete

	hxHashTableNodeBase* m_next; // Embedded singly linked list
};

// ----------------------------------------------------------------------------
// hxHashTable - See top of this file for description.
//
// Node must be a subclass of hxHashTableNode with the interface described above.
// If non-zero HashBits configures the size of the hash table to be HashBits^2.
// Otherwise use set_hash_bits() to configure hash bits dynamically.

template<typename N, uint32_t HashBits=hxAllocatorDynamicCapacity>
class hxHashTable {
public:
	typedef N Node;
	typedef typename Node::Key Key;
	typedef uint32_t size_type;

	// A ForwardIterator.  Iteration is O(nodes + (1u << HashBits)).  Iterator only
	// invalidated by removal of Node referenced.  Not currently bound to
	// std::iterator_traits or std::forward_iterator_tag. 
	class const_iterator
	{
	public:
		HX_INLINE const_iterator(const hxHashTable* tbl)
			: m_hashTable((hxHashTable*)tbl), m_nextIndex(0u), m_currentNode(hx_null) { next(); }
		HX_INLINE const_iterator() : m_hashTable(hx_null), m_nextIndex(0u), m_currentNode(hx_null) { } // end
		HX_INLINE const_iterator& operator++() {
			hxAssertMsg(m_currentNode, "iterator invalid"); // !end
			if (!(m_currentNode = (Node*)m_currentNode->m_next)) {
				next();
			}
			return *this;
		}
		HX_INLINE const_iterator operator++(int) { const_iterator t(*this); operator++(); return t; }
		HX_INLINE bool operator==(const const_iterator& rhs) const { return m_currentNode == rhs.m_currentNode; }
		HX_INLINE bool operator!=(const const_iterator& rhs) const { return m_currentNode != rhs.m_currentNode; }
		HX_INLINE const Node& operator*() const { return *m_currentNode; }
		HX_INLINE const Node* operator->() const { return m_currentNode; }

	protected:
		HX_INLINE void next() {
			hxAssert(m_hashTable && !m_currentNode);
			while (m_nextIndex < m_hashTable->m_table.getCapacity()) {
				if (Node* n = m_hashTable->m_table.getStorage()[m_nextIndex++]) {
					m_currentNode = n;
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
		HX_INLINE iterator(hxHashTable* tbl) : const_iterator(tbl) { }
		HX_INLINE iterator() { }
		HX_INLINE iterator& operator++() { const_iterator::operator++(); return *this; }
		HX_INLINE iterator operator++(int) { iterator t(*this); const_iterator::operator++(); return t; }
		HX_INLINE Node& operator*() const { return *this->m_currentNode; }
		HX_INLINE Node* operator->() const { return this->m_currentNode; }
	};

	HX_INLINE explicit hxHashTable() { m_size = 0u; }
	HX_INLINE ~hxHashTable() { clear(); }

	HX_INLINE const_iterator begin() const { return const_iterator(this); }
	HX_INLINE iterator begin() { return iterator(this); }
	HX_INLINE const_iterator cbegin() const { return const_iterator(this); }

	HX_INLINE const_iterator end() const { return const_iterator(); }
	HX_INLINE iterator end() { return iterator(); }
	HX_INLINE const_iterator cend() const { return const_iterator(); }

	HX_INLINE uint32_t size() const { return m_size; }
	HX_INLINE bool empty() const { return m_size != 0u; }

	// Returns Node& for key.  Any allocations use hxMemoryManagerId_Current and
	// HX_ALIGNMENT_MASK.
	HX_INLINE Node& operator[](const Key& key) { return insert_unique(key); }

	// Returns a node containing key if any or allocates and returns a new one.
	HX_INLINE Node& insert_unique(	const Key& key,
									hxMemoryManagerId allocatorId=hxMemoryManagerId_Current,
									uintptr_t alignmentMask=HX_ALIGNMENT_MASK) {
		uint32_t hash = Node::hash(key);
		Node** pos = getBucket(hash);
		for (Node* n = *pos; n; n = (Node*)n->m_next) {
			if (Node::keyEqual(*n, key, hash)) {
				return *n;
			}
		}
		Node* n = ::new(hxMallocExt(sizeof(Node), allocatorId, alignmentMask))Node(key, hash);
		n->m_next = *pos;
		*pos = n;
		++m_size;
		return *n;
	}

	// Inserts a node.  Allows multiple nodes of the same Key.
	HX_INLINE void insert_node(Node* node) {
		hxAssert(node != hx_null);
		uint32_t hash = node->hash();
		Node** pos = getBucket(hash);
		node->m_next = *pos;
		*pos = node;
		++m_size;
	}

	// Returns a Node matching key if any.  If previous is non-null it must be
	// a node previously returned from find() with the same key and that has
	// not been removed.  Then find() will return a subsequent node if any.
	HX_INLINE Node* find(const Key& key, const Node* previous=hx_null) {
		if (!previous) {
			uint32_t hash = Node::hash(key);
			for (Node* n = *getBucket(hash); n; n = (Node*)n->m_next) {
				if (Node::keyEqual(*n, key, hash)) {
					return n;
				}
			}
		}
		else {
			hxAssert(Node::keyEqual(*previous, key, Node::hash(key)));
			uint32_t hash = previous->hash();
			for (Node* n = (Node*)previous->m_next; n; n = (Node*)n->m_next) {
				if (Node::keyEqual(*n, key, hash)) {
					return n;
				}
			}
		}
		return hx_null;
	}

	HX_INLINE const Node* find(const Key& key, const Node* previous=hx_null) const {
		// Call non-const version for brevity.
		return const_cast<hxHashTable*>(this)->find(key, previous);
	}

	HX_INLINE uint32_t count(const Key& key) const {
		uint32_t total = 0u;
		uint32_t hash = Node::hash(key);
		for (const Node* n = *getBucket(hash); n; n = (Node*)n->m_next) {
			if (Node::keyEqual(*n, key, hash)) {
				++total;
			}
		}
		return total;
	}

	// Removes and returns first node with key if any.
	HX_INLINE Node* extract(const Key& key) {
		uint32_t hash = Node::hash(key);
		Node** next = getBucket(hash);
		while (Node* n = *next) {
			if (Node::keyEqual(*n, key, hash)) {
				*next = (Node*)n->m_next;
				--m_size;
				return n;
			}
			next = (Node**)&n->m_next;
		}
		return hx_null;
	}

	// Release all Nodes matching key and call deleter() on every node.
	// Returns the number of nodes released.  Deleter can be functions with signature
	// "void deleter(Node*)" and functors supporting "operator()(Node*)" and with an
	// "operator bool" returning true.
	template<typename Deleter>
	HX_INLINE uint32_t erase(const Key& key, const Deleter& deleter) {
		uint32_t count = 0u;
		uint32_t hash = Node::hash(key);
		Node** next = getBucket(hash);
		while (Node* n = *next) {
			if (Node::keyEqual(*n, key, hash)) {
				*next = (Node*)n->m_next;
				if (deleter) {
					deleter(n);
				}
				++count;
			}
			else {
				next = (Node**)&n->m_next;
			}
		}
		m_size -= count;
		return count;
	}

	HX_INLINE uint32_t erase(const Key& key) { return erase(key, hxDeleter()); }

	HX_INLINE uint32_t release_key(const Key& key) { return erase(key, (void(*)(Node*))0); }

	// Releases all Nodes and calls deleter() on every node.  Deleter can be
	// function pointers with signature "void deleter(Node*)" or functors
	// supporting "operator()(Node*) and operator (bool)."
	template<typename Deleter>
	HX_INLINE void clear(const Deleter& deleter) {
		if (deleter) {
			if (m_size != 0u) {
				Node** itEnd = m_table.getStorage() + m_table.getCapacity();
				for (Node** it = m_table.getStorage(); it != itEnd; ++it) {
					if (Node* n = *it) {
						*it = 0;
						while (Node* t = n) {
							n = (Node*)n->m_next;
							deleter(t);
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

	HX_INLINE void clear() { clear(hxDeleter()); }

	HX_INLINE void release_all() {
		if (m_size != 0u) {
			::memset(m_table.getStorage(), 0x00, sizeof(Node*) * m_table.getCapacity());
			m_size = 0u;
		}
	}

	HX_INLINE uint32_t bucket_count() const { return m_table.getCapacity(); };

	// Sets bucket count to be 1 << bits.  Only for use with hxAllocatorDynamicCapacity.
	HX_INLINE void set_hash_bits(uint32_t bits) { return m_table.setHashBits(bits); };

	HX_INLINE float load_factor() const { return (float)m_size / (float)bucket_count(); }

	// Returns size of largest bucket.
	uint32_t load_max() const {
		// An unallocated table will be ok.
		uint32_t max=0u;
		const Node*const* itEnd = m_table.getStorage() + m_table.getCapacity();
		for (const Node*const* it = m_table.getStorage(); it != itEnd; ++it) {
			uint32_t count=0u;
			for (const Node* n = *it; n; n = (const Node*)n->m_next) {
				++count;
			}
			max = hxMax(max, count);
		}
		return max;
	}

private:
	HX_STATIC_ASSERT(HashBits <= 31u, "hxHashTable: hash bits must be [0..31]");

	hxHashTable(const hxHashTable&); // Disable copy and assign
	void operator=(const hxHashTable&);

	// Pointer to head of singly-linked list for key's hash value.
	HX_INLINE Node** getBucket(uint32_t hash) {
		uint32_t index = hash >> (32u - m_table.getHashBits());
		hxAssert(index < m_table.getCapacity());
		return m_table.getStorage() + index;
	}

	HX_INLINE const Node*const* getBucket(uint32_t hash) const {
		uint32_t index = hash >> (32u - m_table.getHashBits());
		hxAssert(index < m_table.getCapacity());
		return m_table.getStorage() + index;
	}

	uint32_t m_size;
	hxHashTableInternalAllocator<Node, HashBits> m_table;
};
