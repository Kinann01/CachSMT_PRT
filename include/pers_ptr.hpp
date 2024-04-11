#ifndef CACHSMT_PRT_PERS_PTR_H
#define CACHSMT_PRT_PERS_PTR_H

#include <cstddef> // std::size_t
#include <unordered_map> // std::unordered_map
#include <list> // std::list
#include "file_access.hpp" // header file to read external storage
#include <utility> // std::pair

template <typename ValueType, size_t arity, typename EvictionPolicy>
class cached_container;

template <typename ValueType, size_t arity, typename EvictionPolicy>
class pers_ptr;

class EvictionOldestUnlock;

template <typename ValueType, size_t arity, typename EvictionPolicy>
class pers_ptr {

public:

	// User persistent node
	using userData = persistent_node<ValueType, arity>;

	// The container is a map where internal pointers point to a pair where the .first = the number of pointers pointing at persistent node && .second = the persistent node
	using userDataPair = std::pair<std::size_t, userData>;

	// Explicit constructor to avoid implicit conversions
	explicit pers_ptr(internal_ptr ptr, cached_container<ValueType, arity, EvictionPolicy>* container, userDataPair* p_data) {

		if (!container) {
			throw std::invalid_argument("Container pointer must not be nullptr");
		}

		if (ptr != null_internal_ptr){
			i_ptr = ptr;
			ptr_cache_container = container;
			ptr_NodeData = p_data;

			// If a persistent pointer to a persistent node is created, we increment the number of pointers pointing at it by 1.
			// Since originally when a node is loaded into our cache, number of pointers pointed at it is 0.
			// therefore upon construction the pers ptr, we increment the reference counter by 1.

			(ptr_NodeData->first)++;
		}
		else{

			// For construction of pers_ptr of a null_internal_ptr
			i_ptr = null_internal_ptr;
			ptr_cache_container = nullptr;
			ptr_NodeData = nullptr;
		}
	}

	pers_ptr(const pers_ptr &) = delete;
	pers_ptr &operator=(const pers_ptr &) = delete;

	// Move constructor
	pers_ptr(pers_ptr &&other) noexcept {
		i_ptr = std::exchange(other.i_ptr, null_internal_ptr);
		ptr_cache_container = std::exchange(other.ptr_cache_container, nullptr);
		ptr_NodeData = std::exchange(other.ptr_NodeData, nullptr);
	}

	// Move assignment
	pers_ptr &operator=(pers_ptr &&other) noexcept {
		if (this != &other) {
			if (i_ptr != null_internal_ptr){
				ptr_cache_container->unlockOrDecrementRefCounter(i_ptr, ptr_NodeData);
			}

			i_ptr = std::exchange(other.i_ptr, null_internal_ptr);
			ptr_cache_container = std::exchange(other.ptr_cache_container, nullptr);
			ptr_NodeData = std::exchange(other.ptr_NodeData, nullptr);
		}
		return *this;
	}

	// Validity
	explicit operator bool() const noexcept {
		return i_ptr != null_internal_ptr;
	}

	// Accessing the external storage after loading the root pointer.
	// get_ptr allows us to access through the persistent edges pointing to other nodes.
	pers_ptr get_ptr(size_t index) const {

		if (index >= ptr_NodeData->second.ptr.size()){
			throw std::runtime_error("Index is out of range");
		}

		// Get the new internal_ptr
		internal_ptr newI_ptr = ptr_NodeData->second.ptr[index];

		// get a pointer to the persistent node that the internal_ptr points to
		// getPointer ensures that the node gets loaded if it is not in the cache already

		userDataPair* p_NewActual_Node = ptr_cache_container->getPointer(newI_ptr);

		// Finally construct a persistent pointer to that node
		return pers_ptr<ValueType, arity, EvictionPolicy>(newI_ptr, ptr_cache_container, p_NewActual_Node);
	}

	// Operators for accessing
	const ValueType &operator*() const {
      return ptr_NodeData->second.value;
	}

	const ValueType *operator->() const {
		return &(ptr_NodeData->second.value);
	}

	// Destructor
	// Once a pers_ptr of a persistent node is destroyed,
	// we decrement or possibly unlock the node in the cache.
	// Noexcept since the body will never throw an exception.
	~pers_ptr() noexcept {
		if (i_ptr != null_internal_ptr){
			ptr_cache_container->unlockOrDecrementRefCounter(i_ptr, ptr_NodeData);
		}
	}

private:
	internal_ptr i_ptr;
	cached_container<ValueType, arity, EvictionPolicy>* ptr_cache_container;
	userDataPair* ptr_NodeData;
};

template <typename ValueType, size_t arity, typename EvictionPolicy>
class cached_container {

public:

	// User persistent node
	using userData = persistent_node<ValueType, arity>;

	// The container is a map where internal pointers point to a pair where the .first = the number of nodes pointing at persistent node && .second = the persistent node
	using userDataPair = std::pair<std::size_t, userData>;

	cached_container(size_t size, file_descriptor fd) : maxSize(size), file_d(fd), policy(), file_access_instance(){ }

	pers_ptr<ValueType, arity, EvictionPolicy> root_ptr() {

		auto ptr_NodeData = getPointer(root_internal_ptr);
		return pers_ptr<ValueType, arity, EvictionPolicy>(root_internal_ptr, this, ptr_NodeData);
	}
	pers_ptr<ValueType, arity, EvictionPolicy> null_ptr() {
		auto ptr_NodeData = getPointer(null_internal_ptr);
		return pers_ptr<ValueType, arity, EvictionPolicy>(null_internal_ptr, this, ptr_NodeData);
	}

private:

	struct data_{
		userDataPair userDataPair_;
		typename EvictionPolicy::hint_type hint_type_{};
	};

	// Cache_container class allows pers_ptr class to access its private methods and fields.
	// This is necessary as methods like increment/decrement, unlock, getPointer are used by every pers_ptr which needs access to the cache and its elements.
	friend class pers_ptr<ValueType, arity, EvictionPolicy>;

	size_t maxSize;
	const file_descriptor file_d;
	EvictionPolicy policy;
	std::unordered_map<internal_ptr, data_> cache;
	file_access<userData> file_access_instance;


	// Load into the cache container
	void load(internal_ptr ptr) {

		if (ptr == null_internal_ptr) {
			throw std::runtime_error("Unable to load null internal pointer");
		}

		// Release if cache is full
		if (cache.size() >= maxSize) {
			auto victim = policy.release();
			cache.erase(victim);
		}

		// then read from the external storage and load into the cache.
		userData node_data;
		bool read = file_access_instance.read(file_d, ptr, node_data);

		if (!read){
			throw std::runtime_error("Not able to read data");
		}
		else{
			policy.load(ptr);
			// Create a pair which says (count = 0, node)
			// this counter of the corresponding node will be incremented/decremented
			// depending on how many pers_ptr pointing at it are created/destroyed
			data_ data_in_map;
			data_in_map.userDataPair_ = std::make_pair(0, node_data);
			cache[ptr] = data_in_map;
		}
	}

	// Get a pointer pointing to the node in the cache
	userDataPair* getPointer(internal_ptr ptr) {

		if (ptr == null_internal_ptr){
			return nullptr;
		}
		else{

			auto it = cache.find(ptr);
			if (it != cache.end()){

				// if it is already in the cache but has 0 pointers to it, then re-lock it
				if (it->second.userDataPair_.first == 0){
					auto hint = cache[ptr].hint_type_;
					policy.relock(ptr, hint);
				}

				// If it is in the cache container, return a pointer to it
				return &(cache[ptr].userDataPair_);
			}
			else{
				// else load it and then return a pointer to it.
				load(ptr);
				return &(cache[ptr].userDataPair_);
			}
		}

	}

	// Decrement reference counter or even unlock in the cache container of the count became 0.
	void unlockOrDecrementRefCounter(internal_ptr ptr, userDataPair* p_data) {

		// if this function is called and the reference counter of a node was 1 then it will become 0 and is therefore up for releasing
		if (p_data->first == 1){
			typename EvictionPolicy::hint_type hint = policy.unlock(ptr);
			auto it = cache.find(ptr);
			if (it != cache.end()){
				cache[ptr].hint_type_ = hint;
			}
		}

		(p_data->first)--;;
	}
};

class EvictionOldestUnlock {
public:

	using hint_type = std::list<internal_ptr>::iterator;

	EvictionOldestUnlock() = default;

	void load(internal_ptr) {

	}

	hint_type unlock(internal_ptr ptr) noexcept {

		// If unlocked, add to the end of the list.
		order_unlocked_nodes.emplace_back(ptr);
		auto itr = std::prev(order_unlocked_nodes.end());
		return itr;
	}

	void relock(internal_ptr, hint_type hint) noexcept {
		order_unlocked_nodes.erase(hint);
	}


	internal_ptr release() noexcept {

		// Find the oldest unlocked and pop it from the list.
		auto victim = order_unlocked_nodes.front();
		order_unlocked_nodes.pop_front();
		return victim;
	}

private:

	std::list<internal_ptr> order_unlocked_nodes;
};

#endif //CACHSMT_PRT_PERS_PTR_H
