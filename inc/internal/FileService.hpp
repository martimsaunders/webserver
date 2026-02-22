#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <fstream>

enum Status 
{
	FILE_OK,         // Exists and readable
	FILE_NOT_FOUND,  // Does not exist
	FILE_FORBIDDEN,  // Exists but not readable
	FILE_ERROR       // Other errors
};

struct FileInfo
{
	Status status;
	bool isDirectory;
	bool isRegularFile;
	bool readable;
    bool writable;
	off_t size;
};

// Service Class (stateless)
class FileService
{
	public:
		static FileInfo getFileInfo(const std::string& path);
		static bool pathExists(const std::string& path);
		static bool readFile(const std::string& path, std::string& content);
		static bool writeFile(const std::string& path, const std::string& content);
		static bool deleteFile(const std::string& path);
		static bool listDirectory(const std::string& path, std::vector<std::string>& entries);
};
