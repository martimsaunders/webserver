#include "../inc/ResponseBuilder.hpp"

static std::string toString(int n)
{
    std::ostringstream oss;
    oss << n;
    return (oss.str());
}

std::string ResponseBuilder::escapeHtml(const std::string& input)
{
    std::string out;
    out.reserve(input.size());
    for (size_t i = 0; i < input.size(); ++i)
    {
        switch (input[i])
        {
            case '&': out += "&amp;";  break;
            case '<': out += "&lt;";   break;
            case '>': out += "&gt;";   break;
            case '"': out += "&quot;"; break;
            case '\'': out += "&#39;"; break;
            default: out += input[i];  break;
        }
    }
    return out;
}

// Helpers
int ResponseBuilder::statusFromInfo(const FileInfo& info)
{
    switch (info.status)
    {
        case FILE_NOT_FOUND:   return 404; 
        case FILE_FORBIDDEN:   return 403; 
        default:       		   return 500;
    }
}

std::string ResponseBuilder::resolveReasonPhrase(int statusCode)
{
    switch (statusCode)
    {
        case 200: return "OK";
        case 201: return "Created";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 413: return "Payload Too Large";
        case 500: return "Internal Server Error";
        default:  return "Unknown";
    }
}


std::string ResponseBuilder::loadErrorPage(int statusCode, const ServerConfig& serverConfig)
{
    const std::map<int, std::string>& errorPages = serverConfig.error_pages;
    std::map<int, std::string>::const_iterator it = errorPages.find(statusCode);

    if (it != errorPages.end())
    {
        std::string content;
        if (FileService::readFile(it->second, content))
            return content;
    }

    std::ostringstream body;
    body << "<html><body><h1>"
         << statusCode << " "
         << resolveReasonPhrase(statusCode)
         << "</h1></body></html>";

    return body.str();
}

std::string ResponseBuilder::getMimeType(const std::string& path)
{
    size_t dotPos = path.rfind('.');
    if (dotPos == std::string::npos)
        return "application/octet-stream";

    std::string ext = path.substr(dotPos + 1);
	return Mime::getType(ext);
}

// Factory Methods
HttpResponse ResponseBuilder::buildErrorResponse(int statusCode, const ServerConfig& serverConfig)
{
    std::string body = loadErrorPage(statusCode, serverConfig);
    return buildSimpleResponse(statusCode, body, "text/html");
}

HttpResponse ResponseBuilder::buildErrorResponse(const FileInfo& info, const ServerConfig& serverConfig)
{
    int statusCode = statusFromInfo(info);
    return buildErrorResponse(statusCode, serverConfig);
}

HttpResponse ResponseBuilder::buildRedirectResponse(const Location& location)
{
    HttpResponse res;
    res.setStatusCode(location.redirect_code);
	res.setReasonPhrase(resolveReasonPhrase(location.redirect_code));
    res.addHeader("Location", location.redirect_target);
    res.setBody("<html><body>Redirecting to <a href=\"" + escapeHtml(location.redirect_target) + "\">" + escapeHtml(location.redirect_target) + "</a></body></html>");
    res.addHeader("Content-Length", toString(res.getBody().size()));
    res.addHeader("Content-Type", "text/html");
    return res;
}

HttpResponse ResponseBuilder::buildSimpleResponse(int statusCode, const std::string& body, const std::string& contentType)
{
    HttpResponse res;

    res.setStatusCode(statusCode);
    res.setReasonPhrase(resolveReasonPhrase(statusCode));
    res.setBody(body);

    res.addHeader("Content-Length", toString(body.size()));
    res.addHeader("Content-Type", contentType);

    return res;
}


HttpResponse ResponseBuilder::buildFileResponse(const std::string& fileContent, const std::string& filePath)
{
    HttpResponse res;
    res.setStatusCode(200);
	res.setReasonPhrase(resolveReasonPhrase(res.getStatusCode()));
    res.setBody(fileContent);
    res.addHeader("Content-Length", toString(fileContent.size()));
    res.addHeader("Content-Type", getMimeType(filePath));
    return res;
}

HttpResponse ResponseBuilder::buildAutoindexResponse(const std::string& path, const std::vector<std::string>& entries)
{
	std::ostringstream body;
    body << "<html>\n\t<body>\n\t\t<h1>Index of " << path << "</h1>\n\t\t<ul>";
    for (size_t i = 0; i < entries.size(); ++i)
        body << "\n\t\t\t<li>" << escapeHtml(entries[i]) << "</li>";
    body << "\n\t\t</ul>\n\t</body>\n</html>";
    return buildSimpleResponse(200, body.str(), "text/html");
}
