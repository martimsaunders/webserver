#include "../inc/HttpRequest.hpp"

HttpRequest::HttpRequest() : method(""), uri(""), version(""), headers(), body("") {}
	
// Getters
std::string HttpRequest::getMethod() const { return method; }
std::string HttpRequest::getUri() const { return uri; }
std::string HttpRequest::getVersion() const { return version; }
std::map<std::string, std::string> HttpRequest::getHeaders() const { return headers; }
std::string HttpRequest::getBody() const { return body; }

//Setters
void HttpRequest::setMethod(const std::string& m) { method = m; }
void HttpRequest::setUri(const std::string& u) { uri = u; }
void HttpRequest::setVersion(const std::string& v) { version = v; }
void HttpRequest::addHeader(const std::string& key, const std::string& value) {  headers[key] = value; }
void HttpRequest::setBody(const std::string& b) { body = b; }


bool HttpRequest::hasHeader(const std::string& key) const
{
	//lowercase keys during parsing
	return headers.find(key) != headers.end();
}

//to distinguish between non existant headers and empty ones
bool HttpRequest::tryGetHeader(const std::string& key, std::string& value) const
{
    std::map<std::string, std::string>::const_iterator it = headers.find(key);

    if (it == headers.end())
        return false;

    value = it->second;
    return true;
}

std::string HttpRequest::extensionFromContentType() const
{
    std::string contentType;
    tryGetHeader("content-type", contentType);
    return Mime::getExtension(contentType);
}

std::string HttpRequest::sanitizeFilename(const std::string& raw)
{
    std::string cleaned;
    cleaned.reserve(raw.size());

    for (size_t i = 0; i < raw.size(); ++i)
    {
        const char c = raw[i];
        const bool safe =
            (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') ||
            c == '.' || c == '_' || c == '-';
        if (safe)
            cleaned += c;
        else
            cleaned += '_';
    }

    while (!cleaned.empty() && cleaned[0] == '.')
        cleaned.erase(0, 1);

    if (cleaned.empty())
        return "";

    return cleaned;
}

std::string HttpRequest::filenameFromContentDisposition() const
{
    std::string header;
	tryGetHeader("content-disposition", header);

    if (header.empty())
        return "";

    size_t keyPos = header.find("filename=");
    if (keyPos == std::string::npos)
        return "";

    size_t valueStart = keyPos + 9;
    std::string rawName;
    if (valueStart < header.size() && header[valueStart] == '"')
    {
        ++valueStart;
        size_t valueEnd = header.find('"', valueStart);
        if (valueEnd == std::string::npos)
            return "";
        rawName = header.substr(valueStart, valueEnd - valueStart);
    }
    else
    {
        size_t valueEnd = header.find(';', valueStart);
        if (valueEnd == std::string::npos)
            valueEnd = header.size();
        rawName = header.substr(valueStart, valueEnd - valueStart);
    }

    if (rawName.empty())
        return "";

    // Keep basename only, discard any client-supplied path segments.
    size_t slashPos = rawName.find_last_of("/\\");
    if (slashPos != std::string::npos)
        rawName = rawName.substr(slashPos + 1);

    return sanitizeFilename(rawName);
}
