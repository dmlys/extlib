#pragma once
#include "aci_string.h"

/// подключение заголовочных файлов yaml отдается пользователю.

namespace YAML
{
	template <>
	struct convert<ext::aci_string>
	{
		static bool decode(const Node & node, ext::aci_string & rhs)
		{
			if (!node.IsScalar())
				return false;
			auto val = node.Scalar();
			rhs.assign(val.data(), val.length());
			return true;
		}

		static Node encode(ext::aci_string const & rhs)
		{
			Node node;
			node = std::string(rhs.data(), rhs.length());
			return node;
		}
	};
}
