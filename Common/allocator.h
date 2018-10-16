#ifndef NUMCPP_ALLOCATOR_H_
#define NUMCPP_ALLOCATOR_H_

#include <memory>

namespace np {

struct heap_allocator
{
	static std::shared_ptr<void> allocate(int size)
	{
		return std::shared_ptr<void>(new char[size], free);
	}

	static void free(void *ptr)
	{
		delete[] ptr;
	}
};

} // namespace np

#endif // NUMCPP_ALLOCATOR_H_
