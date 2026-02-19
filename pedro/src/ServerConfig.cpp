#include "../inc/ServerConfig.hpp"

//only checks for longest location path prefix that is contained in the uri
const LocationConfig* ServerConfig::findLocation(const std::string& uri) const
{
    // Find the longest matching prefix
    const LocationConfig* bestMatch = NULL;
    size_t longest = 0;

    for (std::vector<LocationConfig>::const_iterator it = locations.begin(); it != locations.end(); ++it)
    {
        const std::string& locPath = it->path;
        if (locPath.empty())
            continue;

        if (uri.compare(0, locPath.size(), locPath) != 0)
            continue;

        bool boundaryOk = false;
        if (locPath == "/")
            boundaryOk = true;
        else if (uri.size() == locPath.size())
            boundaryOk = true;
        else if (locPath[locPath.size() - 1] == '/')
            boundaryOk = true;
        else if (uri[locPath.size()] == '/')
            boundaryOk = true;

        if (boundaryOk && locPath.size() > longest)
        {
            bestMatch = &(*it);
            longest = locPath.size();
        }
    }

    return bestMatch;
}
