#pragma once
#include <string>
#include <map>

class Mime
{
public:
    static std::string getType(const std::string& extension);
    static std::string getExtension(const std::string& contentType);

private:
    static std::map<std::string, std::string> createExtToMime();
    static std::map<std::string, std::string> createMimeToExt();
};
