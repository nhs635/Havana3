#ifndef NUMCPP_ARRAY_H_
#define NUMCPP_ARRAY_H_

#include "allocator.h"
#include "array_functions.h"

#include <iostream>
#include <array>
#include <complex>

namespace np {

	struct WholeRange
	{
	};

	struct Range
	{
		int from, to;

		Range(int from, int to) : from(from), to(to) { }
	};

	inline WholeRange _colon()
	{
		return WholeRange();
	}

	inline Range _colon(int from, int to)
	{
		return Range(from, to);
	}

	template <size_t N>
	inline int product(const std::array<int, N> &array)
	{
		// TODO: do not use runtime loop
		int result = 1;

		for (int i = 0; i < N; i++)
			result *= array[i];

		return result;
	}

	template <size_t N>
	inline std::array<int, N> zero_array()
	{
		std::array<int, N> result;

		for (int i = 0; i < N; i++)
			result[i] = 0;

		return std::move(result);
	}

	// TODO: variadic template
	inline std::array<int, 1> make_array(int size0)
	{
		std::array<int, 1> result;
		result[0] = size0;
		return result;
	}

	inline std::array<int, 2> make_array(int size0, int size1)
	{
		std::array<int, 2> result;
		result[0] = size0;
		result[1] = size1;
		return result;
	}

	inline std::array<int, 3> make_array(int size0, int size1, int size2)
	{
		std::array<int, 3> result;
		result[0] = size0;
		result[1] = size1;
		result[2] = size2;
		return result;
	}

	template <size_t N>
	inline std::array<int, N> make_stride(const std::array<int, N> &size)
	{
		// TODO do not use runtime loop
		std::array<int, N> stride;

		stride[0] = 1;
		for (int i = 1; i < N; i++)
			stride[i] = stride[i - 1] * size[i - 1];

		return std::move(stride);
	}



	template <typename T, size_t Dim = 1, class Allocator = heap_allocator>
	struct Array
	{
	public:
		typedef T value_type;
		typedef std::array<int, Dim> size_type;
		typedef std::array<int, Dim> stride_type;

		//private:
		int _length;
		size_type _size;
		stride_type _stride;
		std::shared_ptr<void> _address;
		value_type *_origin;

	public:
		Array() :
			_length(0),
			_size(zero_array<Dim>()),
			_stride(make_stride(zero_array<Dim>())),
			_address(),
			_origin(nullptr)
		{
		}

		Array(const size_type &size) :
			_length(product(size)),
			_size(size),
			_stride(make_stride(size)),
			_address(Allocator::allocate(product(size) * sizeof(T))),
			_origin(nullptr)
		{
			_origin = reinterpret_cast<T *>(_address.get());
		}

		explicit Array(int size0) :
			_length(size0),
			_size(make_array(size0)),
			_stride(make_stride<1>(make_array(size0))),
			_address(Allocator::allocate(size0 * sizeof(T))),
			_origin(nullptr)
		{
			static_assert(Dim == 1, "This function is only for Array<T, 1>");
			_origin = reinterpret_cast<T *>(_address.get());
		}

		Array(int size0, int size1) :
			_length(size0 * size1),
			_size(make_array(size0, size1)),
			_stride(make_stride<2>(make_array(size0, size1))),
			_address(Allocator::allocate(size0 * size1 * sizeof(T))),
			_origin(nullptr)
		{
			static_assert(Dim == 2, "This function is only for Array<T, 2>");
			_origin = reinterpret_cast<T *>(_address.get());
		}

		Array(value_type *ptr, int size0) :
			_length(size0),
			_size(make_array(size0)),
			_stride(make_stride<1>(make_array(size0))),
			_address(),
			_origin(ptr)
		{
			static_assert(Dim == 1, "This function is only for Array<T, 1>");
		}

		Array(value_type *ptr, int size0, int size1) :
			_length(size0 * size1),
			_size(make_array(size0, size1)),
			_stride(make_stride<2>(make_array(size0, size1))),
			_address(),
			_origin(ptr)
		{
			static_assert(Dim == 2, "This function is only for Array<T, 2>");
		}

        
		// copy constructor (shallow copy)
		explicit Array(const Array &other) :
			_length(other._length),
			_size(other._size),
			_stride(other._stride),
			_address(other._address), // add refrence count here
			_origin(other._origin)
		{
		}

		// copy assign (shallow copy)
		const Array &operator=(const Array &other)
		{
			_length = other._length;
			_size = other._size;
			_stride = other._stride;
			_address = other._address; // add refrence count here
			_origin = other._origin;

			return *this;
		}

		// move constructor
		Array(Array &&other) :
			_length(other._length),
			_size(std::move(other._size)),
			_stride(std::move(other._stride)),
			_address(std::move(other._address)),
			_origin(other._origin)
		{
			other._length = 0;
			other._origin = nullptr;
		}

		// move assign
		const Array &operator=(Array &&other)
		{
			_length = other._length;
			_size = std::move(other._size);
			_stride = std::move(other._stride);
			_address = std::move(other._address);
			_origin = other._origin;

			other._length = 0;
			other._origin = nullptr;

			return *this;
		}

		int itemSize() const
		{
			return sizeof(value_type);
		}

		int length() const
		{
			return _length;
		}

		int ndims() const
		{
			return Dim;
		}

		const size_type &size() const
		{
			return _size;
		}

		int size(int dim) const
		{
			return _size[dim];
		}

		const stride_type &strides() const
		{
			return _stride;
		}

		int stride(int dim) const
		{
			return _stride[dim];
		}

		value_type *raw_ptr()
		{
			return _origin;
		}

		const value_type *raw_ptr() const
		{
			return _origin;
		}

		operator value_type *()
		{
			return raw_ptr();
		}

		operator const value_type *() const
		{
			return raw_ptr();
		}

		// Accessing elements

		T &at(int index0)
		{
			return _origin[index0 * stride(0)];
		}

		const T &at(int index0) const
		{
			return _origin[index0 * stride(0)];
		}

		T &operator()(int index0) { return at(index0); }
		const T &operator()(int index0) const { return at(index0); }
		T &operator[](int index0) { return at(index0); }
		const T &operator[](int index0) const { return at(index0); }

		T &at(int index0, int index1)
		{
			static_assert(Dim >= 2, "Array dimension bounds error");
			return _origin[index0 * stride(0) + index1 * stride(1)];
		}

		const T &at(int index0, int index1) const
		{
			static_assert(Dim >= 2, "Array dimension bounds error");
			return _origin[index0 * stride(0) + index1 * stride(1)];
		}

		T &operator()(int index0, int index1) { return at(index0, index1); }
		const T &operator()(int index0, int index1) const { return at(index0, index1); }

	public:
		//friend Array<T, 1> slice(const Array<T> &array, const Range &range);
	};

	template <typename T>
	inline void setSize(Array<T, 1> &array, int size0)
	{
		if (array.size(0) != size0)
			array = Array<T, 1>(size0);
	}

	template <typename T>
	inline void setSize(Array<T, 2> &array, int size0, int size1)
	{
		if (array.size(0) != size0 || array.size(1) != size1)
			array = Array<T, 2>(size0, size1);
	}

	template <typename T>
	inline Array<T, 1> slice(const Array<T> &array, const Range &range)
	{
		// TODO: check range
		Array<T, 1> result;

		result._length = range.to - range.from;
		result._size = make_array(range.to - range.from);
		result._stride = array.strides();
		result._address = array._address; // add reference count here
		result._origin = array._origin + range.from; // new origin with offset

		return result;
	}

	template <typename T>
	inline Array<T, 2> slice(const Array<T, 2> &array, const Range &range0, const Range &range1)
	{
		// TODO: check range
		Array<T, 2> result;

		// TODO: implement

		return result;
	}

	template <typename T>
	inline Array<T, 1> slice(Array<T, 2> &array, const WholeRange &range0, int index1)
	{
		Array<T, 1> result;

		result._length = array.size(0);
		result._size = make_array(array.size(0));
		result._stride = make_array(array.stride(0));
		result._address = array._address; // add reference count here
		result._origin = &array.at(0, index1);

		return result;
	}

	using Uint8Array		 = Array<uint8_t>;
	using Uint8Array2		 = Array<uint8_t, 2>;

    using Uint16Array        = Array<uint16_t>;
    using Uint16Array2       = Array<uint16_t, 2>;

	using Uint32Array		 = Array<uint32_t>;
	using Uint32Array2		 = Array<uint32_t, 2>;

    using FloatArray         = Array<float>;
    using FloatArray2        = Array<float, 2>;

	using ComplexFloatArray  = Array<std::complex<float>>;
    using ComplexFloatArray2 = Array<std::complex<float>, 2>;

} // namespace np

#endif // NUMCPP_ARRAY_H_
