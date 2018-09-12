import qbs
import qbs.Environment


Project
{
    property bool with_openssl: false
    property bool with_zlib:    false

    StaticLibrary
    {
        Depends { name: "cpp" }
        cpp.cxxLanguageVersion : "c++17"

        //cpp.defines: additionalDefines
        cpp.cxxFlags: project.additionalCxxFlags
        //cpp.includePaths: project.additionalIncludePaths
        cpp.libraryPaths: project.additionalLibraryPaths
        
        cpp.defines: {
            var defines = []

            if (project.with_openssl)
                defines.push("EXT_ENABLE_OPENSSL")

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
			
			var envIncludes = Environment.getEnv("QBS_THIRDPARTY_INCLUDES")
			if (envIncludes)
			{
				envIncludes = envIncludes.split(qbs.pathListSeparator)
				includes = includes.uniqueConcat(envIncludes)
			}
			
			return includes
		}
        
        Export
        {
            Depends { name: "cpp" }
            cpp.cxxLanguageVersion : "c++17"
            
            cpp.includePaths: ["include"]
            cpp.defines: {
                var defines = []
                
                if (project.with_openssl)
                    defines.push("EXT_ENABLE_OPENSSL")
                
                if (project.with_zlib)
                    defines.push("EXT_ENABLE_CPPZLIB")
                
                return defines;
            }
        }
        
        files: [
            "include/ext/**",
            "src/**",
        ]
        
        excludeFiles: {
            var excludes = [];
            if (qbs.targetOS.contains("windows"))
            {
                excludes.push("include/ext/iostreams/bsdsock*")
                excludes.push("src/bsdsock*")
            }
            else
            {
                excludes.push("include/ext/iostreams/winsock*")
                excludes.push("src/winsock*")
            }

            return excludes
        }
    }
}
