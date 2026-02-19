#include "../inc/Location.hpp"
#include <algorithm>

bool LocationConfig::hasRedirect() const 
{ 
	return has_redirect && !redirect_target.empty(); 
}

bool LocationConfig::isCgiRequest(const std::string& uri) const
{
    size_t pos = uri.rfind('.');
    if (pos == std::string::npos)
        return false;
    std::string ext = uri.substr(pos);
    return cgi.find(ext) != cgi.end();
}

bool LocationConfig::matchesCgi(const std::string& extension) const
{
    return cgi.find(extension) != cgi.end();
}

std::string LocationConfig::resolvePath(const std::string& uri) const
{
    // Map URI path to filesystem path rooted at this location root.
    std::string relative = uri;
    if (relative.find(path) == 0)
        relative = relative.substr(path.size());

    // Ignore query-string/fragment if present.
    size_t queryPos = relative.find('?');
    if (queryPos != std::string::npos)
        relative = relative.substr(0, queryPos);
    size_t fragmentPos = relative.find('#');
    if (fragmentPos != std::string::npos)
        relative = relative.substr(0, fragmentPos);

    while (!relative.empty() && relative[0] == '/')
        relative.erase(0, 1);

    std::vector<std::string> parts;
    size_t start = 0;
    while (start <= relative.size())
    {
        size_t end = relative.find('/', start);
        if (end == std::string::npos)
            end = relative.size();

        std::string token = relative.substr(start, end - start);
        if (!token.empty() && token != ".")
        {
            if (token == "..")
                return ""; // traversal detected
            parts.push_back(token);
        }

        if (end == relative.size())
            break;
        start = end + 1;
    }

    std::string resolved = root;
    if (resolved.empty())
        resolved = ".";

    if (!parts.empty() && resolved[resolved.size() - 1] != '/')
        resolved += "/";

    for (size_t i = 0; i < parts.size(); ++i)
    {
        if (i > 0)
            resolved += "/";
        resolved += parts[i];
    }

    return resolved;
}
