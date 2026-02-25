#include "../../inc/internal/FileService.hpp"
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>     
#include <dirent.h>  
#include <errno.h>
#include <fstream>
#include <sstream>

FileInfo FileService::getFileInfo(const std::string& path)
{
    FileInfo info = {FILE_ERROR, false, false, false, false, false, 0};
    struct stat buffer;

    // Try to stat the requested path
    if (stat(path.c_str(), &buffer) != 0)
    {
        if (errno == ENOENT || errno == ENOTDIR)
			info.status = FILE_NOT_FOUND;

        else if (errno == EACCES)
            info.status = FILE_FORBIDDEN;
        else
            info.status = FILE_ERROR;

        return info;
    }

    // File exists
	info.status        = FILE_OK;
    info.isDirectory   = S_ISDIR(buffer.st_mode);
    info.isRegularFile = S_ISREG(buffer.st_mode);
    info.size          = buffer.st_size;



    // Phase 3 — Permission checks
    if (access(path.c_str(), R_OK) == 0){
        info.readable = true;
    }

    if (access(path.c_str(), W_OK) == 0){
        info.writable = true;
    }

	if (access(path.c_str(), X_OK) == 0){
        info.executable = true;
    }
    return info;
}

bool FileService::pathExists(const std::string& path)
{
    struct stat buffer;
    return stat(path.c_str(), &buffer) == 0;
}

bool FileService::readFile(const std::string& path, std::string& content)
{
    std::ifstream file(path.c_str(), std::ios::in | std::ios::binary);
    if (!file.is_open())
        return false;

    std::ostringstream buffer;
    buffer << file.rdbuf();

    if (file.fail() && !file.eof())
        return false;

    content = buffer.str();
    return true;
}


bool FileService::writeFile(const std::string& path, const std::string& content)
{
	std::ofstream file(path.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
    if (!file.is_open())
        return false;

    file.write(content.c_str(), content.size());

    if (file.fail())
        return false;

    return true;
}

bool FileService::deleteFile(const std::string& path)
{
	return (unlink(path.c_str()) == 0);
}

bool FileService::listDirectory(const std::string& path, std::vector<std::string>& entries)
{
	DIR* dir = opendir(path.c_str());
    if (!dir)
        return false;

    entries.clear();

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL)
    {
        std::string name = entry->d_name;

        if (name != "." && name != "..")
            entries.push_back(name);
    }

    closedir(dir);
    return true;
}
