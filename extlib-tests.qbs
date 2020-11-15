import qbs
import qbs.Environment


CppApplication
{
	type: base.concat("autotest")

	Depends { name: "cpp" }
	Depends { name: "extlib" }
	Depends { name: "ProjectSettings"; required: false }

	cpp.cxxLanguageVersion : "c++17"
	cpp.cxxFlags: project.additionalCxxFlags
	cpp.driverFlags: project.additionalDriverFlags
	cpp.defines: ["BOOST_TEST_DYN_LINK"].uniqueConcat(project.additionalDefines || [])
	cpp.systemIncludePaths: project.additionalSystemIncludePaths
	cpp.includePaths: ["include"].uniqueConcat(project.additionalIncludePaths || [])
	cpp.libraryPaths: project.additionalLibraryPaths


	cpp.dynamicLibraries: 
	{
		var libs = [
			"stdc++fs", "fmt",
			//"boost_system",
			//"boost_test_exec_monitor",
			"boost_locale",
			"boost_unit_test_framework",
		]
		
		if (extlib.with_zlib)
			libs.push("z")
		
		if (extlib.with_openssl)
			libs = libs.concat(["crypto"])
			//libs = libs.concat(["ssl", "crypto"])
		
		if (qbs.toolchain.contains("mingw"))
			libs.push("ssp") // for mingw(gcc) stack protector, _FORTIFY_SOURCE stuff
		
		return libs
	}

	files: [
		"tests/**"
    ]
}
