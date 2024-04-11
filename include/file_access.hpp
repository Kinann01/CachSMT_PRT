// file_access.h
// to be included


#ifndef FILE_ACCESS_H__
#define FILE_ACCESS_H__

#pragma once

#include <array>
#include <cstddef>


using internal_ptr = int ;		// internal persistent ptr === item _index_ within the file (0-based)
constexpr internal_ptr null_internal_ptr = -1 ;		// nullptr
constexpr internal_ptr root_internal_ptr = 0 ;		// ptr to the root item

// persistent data structure
template <typename ValueType, size_t arity>
struct persistent_node {
	ValueType value;
	std::array< internal_ptr, arity> ptr;
};

using file_descriptor = std::array<persistent_node<int, 1>, 5> ;			// some sort of magic

template <typename T>
class file_access {
public:
	file_access() = default;
	bool read(file_descriptor fd, internal_ptr ip, T& item){
        item = fd[ip];
        return true;
    }
private:
	/* IMPLEMENTATION-DEFINED */
};

/* specializations IMPLEMENTATION-DEFINED */

#endif
