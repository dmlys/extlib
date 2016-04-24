#pragma once
#include <boost/regex.hpp>
#include <boost/range.hpp>

/// boost::sub_match integration with boost::range

namespace boost
{
	template< class It >
	struct range_mutable_iterator< sub_match<It> >
	{
		typedef It type;
	};

	template< class It >
	struct range_const_iterator< sub_match<It> >
	{
		typedef It type;
	};

} // namespace 'boost'

namespace boost
{
	// The required functions. These should be defined in
	// the same namespace as 'sub_match', in this case
	// in namespace 'boost'.

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

} // namespace 'boost'