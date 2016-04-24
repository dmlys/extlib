#pragma once
#include <ext/range/input_range_facade.hpp>
#include <istream>
#include <boost/noncopyable.hpp>

namespace ext
{
	template <class Type>
	struct istream_range :
		input_range_facade<istream_range<Type>, Type>,
		boost::noncopyable
	{
	private:
		Type val;
		std::istream & is;

	public:
		void pop_front() { is >> val; }
		Type & front() { return val; }
		bool empty() { return !is; }

		istream_range(std::istream & is) : is(is)
		{
			pop_front();
		}
	};
}