#include "ServerConfig.hpp"

ServerConfig::ServerConfig() : ip("0.0.0.0"), port(80), root("/"), maxBodySize(0), errorPages(), locations() {}

// Getters
const std::string& ServerConfig::getIp() const { return ip; }
int ServerConfig::getPort() const { return port; }
const std::string& ServerConfig::getRoot() const {return root; }
size_t ServerConfig::getMaxBodySize() const { return maxBodySize; }
const std::map<int, std::string>& ServerConfig::getErrorPages() const { return errorPages; }
const std::vector<Location>& ServerConfig::getLocations() const { return locations; }

// Setters
void ServerConfig::setIp(const std::string& ip_) { ip = ip_; }
void ServerConfig::setPort(int p) { port = p; }
void ServerConfig::setRoot(const std::string& r) { root = r; }
void ServerConfig::setMaxBodySize(size_t size) { maxBodySize = size; }
void ServerConfig::addErrorPage(int code, const std::string& path) { errorPages[code] = path; }
void ServerConfig::addLocation(const Location& loc) { locations.push_back(loc); }

//only checks for longest location path prefix that is contained in the uri
const Location* ServerConfig::findLocation(const std::string& uri) const
{
    // Find the longest matching prefix
    const Location* bestMatch = NULL;
    size_t longest = 0;

    for (std::vector<Location>::const_iterator it = locations.begin(); it != locations.end(); ++it)
    {
        const std::string& locPath = it->getPath();
        if (uri.compare(0, locPath.size(), locPath) == 0 && locPath.size() > longest)
        {
            bestMatch = &(*it);
            longest = locPath.size();
        }
    }

    return bestMatch;
}