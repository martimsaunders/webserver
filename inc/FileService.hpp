#pragma once
#include <string>

class FileService
{
	public:
		static bool exists(const std::string& path);
		static bool isDirectory(const std::string& path);
		static std::string readFile(const std::string& path);
		static bool deleteFile(const std::string& path);
};
