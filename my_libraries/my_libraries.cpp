// my_libraries.cpp : Defines the entry point for the application.
//

#include "my_libraries.h"

using namespace std;

struct other_alloc final : std::allocator<int> {};

int main()
{
	bhavesh::detail::matrix::_compressed_data<other_alloc> a({}, 1, 1);
	bhavesh::detail::matrix::_compressed_data<other_alloc> b({}, 1, 1);
	swap(a, b);
	cout << "Hello CMake." << endl;
	cin.get();
	return 0;
}
