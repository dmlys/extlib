#include <ext/FileSystemUtils.hpp>
#include <ext/regex_utils.hpp>

#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/range.hpp>
#include <boost/range/algorithm.hpp>

namespace fs = boost::filesystem;

namespace ext
{
	std::vector<fs::path> FilesByMask(fs::path const & mask)
	{
		if (mask.empty())
			return std::vector<fs::path>();

		fs::path parent = mask.parent_path();
		if (parent.empty())
			parent = ".";

		FsMask rmask = boost::regex(wildcard_to_regex(mask.stem().string()), boost::regex::icase);
		fs::directory_iterator iter(parent), eod;

		std::vector<fs::path> res;
		for (; iter != eod; ++iter) {
			if (rmask(iter->path().stem()))
				res.push_back(iter->path());
		}

		return res;
	}

	bool is_wild_card(fs::path const & path)
	{
		auto const & str = path.native();
		return boost::find_first_of(str, "?*") != boost::end(str);
	}
}
