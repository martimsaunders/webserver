#pragma once
#include <string>
#include <vector>

class Location
{
private:
    std::string path;                 
    std::string root;                 
    std::vector<std::string> allowedMethods;
    bool autoindex;
    std::string indexFile;
    bool uploadEnabled;
    std::string uploadPath;
    std::string redirect;
    std::string cgiExtension;
    std::string cgiPath;

public:
    Location();

    //Getters
    const std::string& getPath() const;
    const std::string& getRoot() const;
    const std::vector<std::string>& getAllowedMethods() const;
    bool getAutoindex() const;
    const std::string& getIndexFile() const;
    bool getUploadEnabled() const;
    const std::string& getUploadPath() const;
    const std::string& getRedirect() const;
    const std::string& getCgiExtension() const;
    const std::string& getCgiPath() const;

    //Helper methods
    bool isMethodAllowed(const std::string& method) const;
    std::string resolvePath(const std::string& uri) const;
    bool needsRedirect() const;
    bool isCgiRequest(const std::string& uri) const;
};
