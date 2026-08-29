#pragma once

#include <filesystem>
#include <string>
#include <cctype>  // for std::tolower

namespace fs = std::filesystem;

namespace utils
{

	bool isFileExists(const char* path) {
		if (!path) return false;
		return fs::exists(path) && fs::is_regular_file(path);
	}

	std::string getFileExtension(const char* path) {
		if (!path) return "";
		fs::path p(path);
		std::string ext = p.extension().string();
		return ext;
	}

	template <typename T>
	bool isFind(const T& v, const std::vector<T>& candicates)
	{
		return std::find(candicates.begin(), candicates.end(), v) != candicates.end();
	}


}