import qbs
import qbs.Environment


Project
{
	property bool with_zlib: false

	StaticLibrary
	{
		Depends { name: "cpp" }
		cpp.cxxLanguageVersion : "c++17"

		//cpp.defines: project.additionalDefines
		//cpp.includePaths: project.additionalIncludePaths
		cpp.systemIncludePaths: project.additionalSystemIncludePaths
		cpp.cxxFlags: project.additionalCxxFlags
		cpp.driverFlags: project.additionalDriverFlags
		cpp.libraryPaths: project.additionalLibraryPaths

		cpp.defines: {
			var defines = []

			if (project.with_zlib)
				defines.push("EXT_ENABLE_CPPZLIB")

			if (project.additionalDefines)
				defines = defines.uniqueConcat(project.additionalDefines)

			return defines
		}

		cpp.includePaths: {
			var includes = ["include"]
			if (project.additionalIncludePaths)
				includes = includes.uniqueConcat(project.additionalIncludePaths)

			return includes
		}

		Export
		{
			Depends { name: "cpp" }
			cpp.cxxLanguageVersion : "c++17"

			cpp.includePaths: ["include"]
			cpp.defines: {
				var defines = []

				if (project.with_zlib)
					defines.push("EXT_ENABLE_CPPZLIB")

				return defines;
			}
		}

		files: [
			"include/ext/**",
			"src/**",
		]
	}
}
