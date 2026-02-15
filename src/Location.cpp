#include "Location.hpp"
#include <algorithm>

Location::Location() 
    : path("/"), root("/"), allowedMethods(), autoindex(false), indexFile("index.html"),
      uploadEnabled(false), uploadPath(""), redirect(""), cgiExtension(""), cgiPath("") {}

// Getters
const std::string& Location::getPath() const { return path; }
const std::string& Location::getRoot() const { return root; }
const std::vector<std::string>& Location::getAllowedMethods() const { return allowedMethods; }
bool Location::getAutoindex() const { return autoindex; }
const std::string& Location::getIndexFile() const { return indexFile; }
bool Location::getUploadEnabled() const { return uploadEnabled; }
const std::string& Location::getUploadPath() const { return uploadPath; }
const std::string& Location::getRedirect() const { return redirect; }
const std::string& Location::getCgiExtension() const { return cgiExtension; }
const std::string& Location::getCgiPath() const { return cgiPath; }

// Setters
void Location::setPath(const std::string& p) { path = p; }
void Location::setRoot(const std::string& r) { root = r; }
void Location::setAllowedMethods(const std::vector<std::string>& methods) { allowedMethods = methods; }
void Location::addAllowedMethod(const std::string& method) { allowedMethods.push_back(method); }
void Location::setAutoindex(bool enabled) { autoindex = enabled; }
void Location::setIndexFile(const std::string& file) { indexFile = file; }
void Location::setUploadEnabled(bool enabled) { uploadEnabled = enabled; }
void Location::setUploadPath(const std::string& path_) { uploadPath = path_; }
void Location::setRedirect(const std::string& url) { redirect = url; }
void Location::setCgiExtension(const std::string& ext) { cgiExtension = ext; }
void Location::setCgiPath(const std::string& path_) { cgiPath = path_; }


bool Location::isMethodAllowed(const std::string& method) const
{
    for (std::vector<std::string>::const_iterator it = allowedMethods.begin(); it != allowedMethods.end(); ++it)
        if (*it == method) return true;
    return false;
}

bool Location::needsRedirect() const { return !redirect.empty(); }

bool Location::isCgiRequest(const std::string& uri) const
{
    if (cgiExtension.empty()) return false;

    // extract extension from URI
    std::string ext;
	// finds last . in the string
    size_t pos = uri.rfind('.');
    if (pos != std::string::npos)
        ext = uri.substr(pos);

    return ext == cgiExtension;
}

std::string Location::resolvePath(const std::string& uri) const
{
    // Map URI (route) to filesystem path based on root
    std::string relative = uri;
	// remove the routing rule and prefix with root
    if (relative.find(path) == 0)
        relative = relative.substr(path.size()); // remove location prefix
    return root + relative;
}
