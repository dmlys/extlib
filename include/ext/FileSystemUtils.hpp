#pragma once
#ifndef FILESYSTEM_UTILS_H_
#define FILESYSTEM_UTILS_H_

#include <vector>
#include <boost/regex.hpp>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/swap.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include <ext/range.hpp>
#include <ext/Errors.hpp>

namespace ext
{
	/// данный класс-предикат предназначен для фильтрации коллекций
	/// boost.filesystem.path / boost.filesystem.directory_entry по заданному регекс выражению
	struct FsMask
	{
		FsMask(boost::regex const & mask) : rx(mask) {};
		FsMask(std::string const & mask) : rx(mask) {};

		bool operator ()(boost::filesystem::path const & item) { return regex_match(item.string(), rx); }
		bool operator ()(boost::filesystem::directory_entry const & item) { return operator()(item.path()); }
	private:
		boost::regex rx;
	};

	/// возвращает набор файлов по указанному пути и маске
	/// например "E:/work/*.txt"
	/// маске должна быть только в конце, т.е. E:/wo*k/*.txt - некорректно
	std::vector<boost::filesystem::path> FilesByMask(boost::filesystem::path const & mask);

	///проверяет содержит ли строка символы wildCard
	bool is_wild_card(boost::filesystem::path const & path);

	/// считывает файл по заданному пути file в контейнер content
	/// в поток reps выдается информация о процессе загрузки файла(в том числе и в случае успеха).
	/// по умолчанию грузит файл в текстовом режиме
	template <class Container>
	bool LoadFile(boost::filesystem::path const & file, Container & content, std::ostream & reps,
	              std::ios_base::openmode mode = std::ios_base::in /*| std::ios_base::text*/)
	{
		static_assert(ext::has_resize_method<Container>::value, "not resizable container");
		static_assert(std::is_same<typename boost::range_value<Container>::type, char>::value, "not a char container");

		boost::filesystem::ifstream ifs(file, mode);
		if (!ifs.is_open())
		{
			//      "Failed to open {1}, {2}
			reps << "Failed to open " << file << ", " << ext::FormatErrno(errno) << std::endl;
			return false;
		}

		ifs.unsetf(std::ios::skipws);
		ifs.seekg(0, std::ios::end);
		std::streamsize fileSize = ifs.tellg();
		ifs.seekg(0);

		Container content_;
		try
		{
			std::make_unsigned<std::streamsize>::type ufs = fileSize; // to silence warning on x64
			if (ufs > content_.max_size())
				throw std::bad_alloc();
			content_.resize(static_cast<std::size_t>(fileSize), 0);
		}
		catch (std::bad_alloc &)
		{
			// "Failed to read {1}, file to big, size is {2,num}
			reps << "Failed to read " << file << ", file to big, size is " << fileSize << std::endl;
			return false;
		}

		ifs.read(ext::data(content_), fileSize);
		if (ifs.bad())
		{
			// "Failed to read {1}, {2}
			reps << "Failed to read " << file << ", " << ext::FormatErrno(errno) << std::endl;
			return false;
		}
		content_.resize(static_cast<std::size_t>(ifs.gcount()));

		boost::swap(content_, content);
		return true;
	}

	template <class Container>
	bool WriteFile(boost::filesystem::path const & file, Container & content, std::ostream & reps,
	              std::ios_base::openmode mode = std::ios_base::in /*| std::ios_base::text*/)
	{
		boost::filesystem::ofstream ifs(file, mode);
		if (!ifs.is_open())
		{
			//      "Failed to open {1}, {2}
			reps << "Failed to open " << file << ", " << ext::FormatErrno(errno) << std::endl;
			return false;
		}
		
		ifs.write(ext::data(content), content.size());
		if (ifs.bad())
		{
			// "Failed to read {1}, {2}
			reps << "Failed to write " << file << ", " << ext::FormatErrno(errno) << std::endl;
			return false;
		}
		
		return true;
	}

	struct FileLoadError : std::runtime_error
	{
		FileLoadError(std::string const & msg) : std::runtime_error(msg) {}
	};

	struct FileWriteError : std::runtime_error
	{
		FileWriteError(std::string const & msg) : std::runtime_error(msg) {}
	};

	template <class Container>
	void LoadFile(boost::filesystem::path const & file, Container & content,
	              std::ios_base::openmode mode = std::ios_base::in /*| std::ios_base::text*/)
	{
		std::stringstream reps;
		if (!LoadFile(file, content, reps, mode))
			throw FileLoadError(reps.str());
	}

	template <class Container>
	void WriteFile(boost::filesystem::path const & file, Container & content,
	               std::ios_base::openmode mode = std::ios_base::in /*| std::ios_base::text*/)
	{
		std::stringstream reps;
		if (!WriteFile(file, content, reps, mode))
			throw FileWriteError(reps.str());
	}


	/// получает полный путь к exe файлу, в рамках которого исполняется код
	boost::filesystem::path GetExePath(int argc, char *argv[]);

	/// аналогично boost::property_tree::read_ini, но считывает файл с помощью
	/// ext::LoadFile, проверяет utf-8 bom(удаялет, если таковой есть)
	/// и после этого передает в boost::property_tree::read_ini
	template <class Ptree>
	void read_ini(boost::filesystem::path filePath, Ptree & pt)
	{
		std::string content;
		LoadFile(filePath, content);
		if (content.compare(0, 3, "\xEF\xBB\xBF", 3) == 0) //starts with bom
			content.erase(0, 3);

		std::stringstream ss(content);
		boost::property_tree::read_ini(ss, pt);
	}
}

#endif