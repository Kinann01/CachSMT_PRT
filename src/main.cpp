#include <iostream>
#include "../include/pers_ptr.hpp"

int main()
{
	std::array<internal_ptr, 1> a0 = {1};
	std::array<internal_ptr, 1> a1 = {2};
	std::array<internal_ptr, 1> a2 = {3};
	std::array<internal_ptr, 1> a3 = {4};
	std::array<internal_ptr, 1> a4 = {null_internal_ptr};

	persistent_node<int, 1> n0 = {0, a0};
	persistent_node<int, 1> n1 = {1, a1};
	persistent_node<int, 1> n2 = {2, a2};
	persistent_node<int, 1> n3 = {3, a3};
	persistent_node<int, 1> n4 = {4, a4};

	std::array<persistent_node<int, 1>, 5> file = {n0, n1, n2, n3, n4};

	cached_container< int, 1, EvictionOldestUnlock> cc(3, file);

	for (auto p = cc.root_ptr(); p; p = p.get_ptr(0))
		std::cout << *p << std::endl;


	return 0;
}