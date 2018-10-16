#ifndef NUMCPP_ARRAY_FUNCTIONS_H_
#define NUMCPP_ARRAY_FUNCTIONS_H_

namespace np {

template <class Array>
bool empty(const Array &array)
{
	return array.raw_ptr() == nullptr || array.length() == 0;
}

template <class Array>
int byteSize(const Array &array)
{
	return array.length() * array.itemSize();
}

} // namespace np

#endif // NUMCPP_ARRAY_FUNCTIONS_H_