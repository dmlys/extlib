#include <ext/FileSystemUtils.hpp>

#include <boost/predef.h>
#include <boost/algorithm/string.hpp>
/************************************************************************/
/*               CurrentExePath implementation                          */
/************************************************************************/
namespace ext
{
	static boost::filesystem::path GetExePath(int argc, char *argv[], boost::system::error_code & ec);

	boost::filesystem::path GetExePath(int argc, char *argv[])
	{
		boost::system::error_code ec;
		auto path = GetExePath(argc, argv, ec);
		if (ec)
			throw boost::system::system_error(ec, "CurrentExePath failed");

		return path;
	}

	/*
	namespace
	{
		static boost::filesystem::path CurrentExePathViaCommandLine(int argc, char *argv[])
		{
			if (argc == 0)
				return "";

			boost::filesystem::path zero = argv[0];
			// путь абсолютный, возвращаем как есть
			if (zero.is_absolute())
				return zero;

			if (zero.has_parent_path())
				return absolute(zero);

			return "";
		}

		static boost::filesystem::path CurrentExePathViaEnvironment(int argc, char *argv[])
		{
			if (argc == 0)
				return "";

			boost::filesystem::path zero = argv[0];

			//ищем через path
			std::string path = getenv("PATH");
			if (path.empty())
				return "";

			for (auto beg = path.begin(), end = std::find(beg, path.end(), ';');
				 beg != end;
				 beg = end, end = std::find(beg, end, ';'))
			{
				boost::filesystem::path inpath(beg, end);
				inpath /= zero;
				if (boost::filesystem::exists(inpath))
					return inpath;
			}

			return "";
		}
	}
	*/
}

/*
http://stackoverflow.com/questions/1023306/finding-current-executables-path-without-proc-self-exe

Some OS-specific interfaces:

Mac OS X: _NSGetExecutablePath() (man 3 dyld)
Linux: readlink /proc/self/exe
Solaris: getexecname()
FreeBSD: sysctl CTL_KERN KERN_PROC KERN_PROC_PATHNAME -1
FreeBSD if it has procfs: readlink /proc/curproc/file (FreeBSD doesn't have procfs by default)
NetBSD: readlink /proc/curproc/exe
DragonFly BSD: readlink /proc/curproc/file
Windows: GetModuleFileName() with hModule = NULL

The portable (but less reliable) method is to use argv[0].
Although it could be set to anything by the calling program,
by convention it is set to either a path name of the executable or a name that was found using $PATH.

Some shells, including bash and ksh,
set the environment variable "_" to the full path of the executable before it is executed.
In that case you can use getenv("_") to get it.
However this is unreliable because not all shells do this,
and it could be set to anything or be left over from a parent process
which did not change it before executing your program.
*/


#if BOOST_OS_WINDOWS
#include <Windows.h>

namespace ext
{
	boost::filesystem::path GetExePath(int argc, char *argv[], boost::system::error_code & ec)
	{
		std::vector<wchar_t> path(MAX_PATH);

		DWORD sz = ::GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
		while (path.size() == sz) // if buffer size is not enough, increase by 2
		{
			path.resize(path.size() * 2);
			sz = ::GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
		}

		if (!sz) {
			ec.assign(GetLastError(), boost::system::system_category());
			return "";
		}

		ec.clear();
		return boost::filesystem::path(path.data(), path.data() + sz);
	}
}

#elif BOOST_OS_LINUX
#include <unistd.h>
#include <limits.h>

namespace ext
{
	boost::filesystem::path GetExePath(int argc, char *argv[], boost::system::error_code & ec)
	{
		char exePath[PATH_MAX];
		
		ssize_t len = ::readlink("/proc/self/exe", exePath, sizeof(exePath));
		if (len == -1 || len == sizeof(exePath))
			len = 0;

		exePath[len] = '\0';

		return boost::filesystem::path(exePath, exePath + len);
	}
}

#elif BOOST_OS_MACOS
#include <mach-o/dyld.h>

namespace ext
{
	boost::filesystem::path GetExePath(int argc, char *argv[], boost::system::error_code & ec)
	{
		char exePath[PATH_MAX];
		uint32_t len = sizeof(exePath);

		if (_NSGetExecutablePath(exePath, &len) != 0) 
		{
			exePath[0] = '\0'; // buffer too small (!)
		}
		else
		{
			// resolve symlinks, ., .. if possible
			auto * canonicalPath = realpath(exePath, NULL);
			if (canonicalPath != NULL)
			{
				strncpy(exePath, canonicalPath, len);
				free(canonicalPath);
			}
		}
	}
}

#elif BOOST_OS_SOLARIS
#include <stdlib.h>
#include <limits.h>

namespace ext
{
	boost::filesystem::path GetExePath(int argc, char *argv[], boost::system::error_code & ec)
	{
		char exePath[PATH_MAX];
		if (realpath(getexecname(), exePath) == NULL)
			exePath[0] = '\0';

		return boost::filesystem::path(exePath);
	}
}

#elif BOOST_OS_BSD_FREE
#include <sys/types.h>
#include <sys/sysctl.h>

namespace ext
{
	boost::filesystem::path GetExePath(int argc, char *argv[], boost::system::error_code & ec)
	{
		int mib[4];  
		mib[0] = CTL_KERN;
		mib[1] = KERN_PROC;
		mib[2] = KERN_PROC_PATHNAME;
		mib[3] = -1;

		char exePath[2048];
		size_t len = sizeof(exePath);

		if (sysctl(mib, 4, exePath, &len, NULL, 0) != 0)
			exePath[0] = '\0';

		return boost::filesystem::path(exePath);
	}
}

#else

#error(Not implmented)

#endif
