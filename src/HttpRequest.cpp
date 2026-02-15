#include "HttpRequest.hpp"

HttpRequest::HttpRequest() : method(""), uri(""), version(""), headers(), body("") {}
	
// Getters
std::string HttpRequest::getMethod() const { return method; }
std::string HttpRequest::getUri() const { return uri; }
std::string HttpRequest::getVersion() const { return version; }

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
