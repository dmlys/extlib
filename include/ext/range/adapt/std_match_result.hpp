#pragma once
#include <regex>
#include <boost/range.hpp>

/// std::sub_match integration with boost::range
namespace boost
{
	template< class It >
	struct range_mutable_iterator< std::sub_match<It> >
	{
		typedef It type;
	};

	template< class It >
	struct range_const_iterator< std::sub_match<It> >
	{
		typedef It type;
	};

} // namespace 'boost'

namespace std
{
	// The required functions. These should be defined in
	// the same namespace as 'sub_match', in this case
	// in namespace 'std'.

	template< class It >
	inline It range_begin(sub_match<It> & x)
	{
		return x.first;
	}

	template< class It >
	inline It range_begin(const sub_match<It> & x)
	{
		return x.first;
	}

	template< class It >
	inline It range_end(sub_match<It> & x)
	{
		return x.second;
	}

	template< class It >
	inline It range_end(const sub_match<It> & x)
	{
		return x.second;
	}

} // namespace 'std'