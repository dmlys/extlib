#pragma once

#include <algorithm>

#include <boost/range.hpp>
#include <boost/variant/variant.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp>

namespace ext {
namespace varalgo {

	template <class Type>
	struct min_visitor : boost::static_visitor<const Type &>
	{
		const Type & a;
		const Type & b;

		min_visitor(const Type & a, const Type & b)
			: a(a), b(b) {}

		template <class Pred>
		inline const Type & operator()(Pred pred) const
		{
			return std::min(a, b, pred);
		}
	};

	template <class Type>
	struct max_visitor : boost::static_visitor<const Type &>
	{
		const Type & a;
		const Type & b;

		max_visitor(const Type & a, const Type & b)
			: a(a), b(b) {}

		template <class Pred>
		inline const Type & operator()(Pred pred) const
		{
			return std::max(a, b, pred);
		}
	};

	template <class Type>
	struct minmax_visitor : boost::static_visitor<std::pair<const Type &, const Type &>>
	{
		const Type & a;
		const Type & b;

		minmax_visitor(const Type & a, const Type & b)
			: a(a), b(b) {}

		template <class Pred>
		inline std::pair<const Type &, const Type &> operator()(Pred pred) const
		{
			return std::minmax(a, b, pred);
		}
	};

	template <class Type, class... VariantTypes>
	inline const Type & min(const Type & a, const Type & b, const boost::variant<VariantTypes...> & pred)
	{
		return boost::apply_visitor(min_visitor<Type> {a, b}, pred);
	}

	template <class Type, class Pred>
	inline const Type & min(const Type & a, const Type & b, Pred pred)
	{
		return std::min(a, b, pred);
	}

	template <class Type, class... VariantTypes>
	inline const Type & max(const Type & a, const Type & b, const boost::variant<VariantTypes...> & pred)
	{
		return boost::apply_visitor(max_visitor<Type> {a, b}, pred);
	}

	template <class Type, class Pred>
	inline const Type & max(const Type & a, const Type & b, Pred pred)
	{
		return std::max(a, b, pred);
	}

	template <class Type, class... VariantTypes>
	inline std::pair<const Type &, const Type &>
		minmax(const Type & a, const Type & b, const boost::variant<VariantTypes...> & pred)
	{
		return boost::apply_visitor(minmax_visitor<Type> {a, b}, pred);
	}

	template <class Type, class Pred>
	inline std::pair<const Type &, const Type &>
		minmax(const Type & a, const Type & b, Pred pred)
	{
		return std::minmax(a, b, pred);
	}
}}