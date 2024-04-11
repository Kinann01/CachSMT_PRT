# Cached Persistent Pointer Library

## Overview
This library provides an implementation of a cache-aware, persistent pointer system (`pers_ptr`) for C++ applications. It is designed to handle objects in memory with efficient caching and eviction policies. The library uses template parameters to allow for customizable value types, arity, and eviction policies.

## Features
- Template-based implementation to support various data types and eviction policies.
- Custom eviction policy implementation with an example of the oldest-unlock eviction policy (`EvictionOldestUnlock`).
- External storage access for persistent data managed through a `file_access` system.

## Prerequisites
- C++20 compliant compiler

## Compilation
Ensure your compiler supports C++20 due to the use of features from this standard:

```sh
g++ -std=c++20 src/main.cpp
```

Then run the executable:

```sh
./a.out
```