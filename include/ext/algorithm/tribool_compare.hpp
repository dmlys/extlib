#pragma once

namespace ext
{
	/// сравнивает и идентифицирует отношение 2 аргументов
	/// -1 if op1 < op2;
	/// 0 if op1 == op2;
	/// +1 if op1 > op2;
	template <class Type>
	int tribool_compare(Type const & op1, Type const & op2)
	{
		if (op1 < op2) return -1;
		if (op2 < op1) return +1;
		return 0;
	}

	/// сравнивает и идентифицирует отношение 2 аргументов
	/// -1 if comp(op1, op2);
	/// +1 if comp(op2, op1);
	/// 0 otherwise
	template <class Type, class Comp>
	int tribool_compare(Type const & op1, Type const & op2, Comp comp)
	{
		if (comp(op1, op2)) return -1;
		if (comp(op2, op1)) return +1;
		return 0;
	}
}