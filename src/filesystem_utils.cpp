#include <ext/filesystem_utils.hpp>
#include <ext/regex_utils.hpp>

#include <boost/range/algorithm.hpp>

namespace ext
{
	std::vector<std::filesystem::path> FilesByMask(const std::filesystem::path & mask)
	{
		if (mask.empty())
			return std::vector<std::filesystem::path>();

		std::filesystem::path parent = mask.parent_path();
		if (parent.empty()) parent = ".";

		fsmask rmask = std::regex(wildcard_to_regex(mask.stem().string()), std::regex::icase);
		std::filesystem::directory_iterator iter(parent), eod;

		std::vector<std::filesystem::path> res;
		for (; iter != eod; ++iter)
		{
			if (rmask(iter->path().stem()))
				res.push_back(iter->path());
		}

		return res;
	}

	bool is_wild_card(const std::filesystem::path & path)
	{
		const auto & str = path.native();
		return boost::find_first_of(str, "?*") != boost::end(str);
	}
}
