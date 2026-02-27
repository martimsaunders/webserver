#include "../../inc/response/HttpResponse.hpp"
#include <sstream>

HttpResponse::HttpResponse() : statusCode(200), body("") {}

// Getters
int HttpResponse::getStatusCode() const { return statusCode; }
std::string HttpResponse::getReasonPhrase() const { return reasonPhrase; }
const std::map<std::string, std::string>& HttpResponse::getHeaders() const { return headers; }
const std::string& HttpResponse::getBody() const { return body; }

// Setters
void HttpResponse::setStatusCode(int code) { statusCode = code; }
void HttpResponse::setReasonPhrase(const std::string &phrase) {reasonPhrase = phrase; }
void HttpResponse::addHeader(const std::string& key, const std::string& value) { headers[key] = value; }
void HttpResponse::setBody(const std::string& content) { body = content; }

// Helper methods
std::string HttpResponse::toString(){
	std::ostringstream oss;
	oss << "HTTP/1.1 " << statusCode << " " << reasonPhrase << "\r\n";

	static const char* preferredOrder[] = {
		"Date",
		"Server",
		"Content-Length",
		"Content-Type",
		"Connection"
	};
	const size_t preferredCount = sizeof(preferredOrder) / sizeof(preferredOrder[0]);

	// Emit preferred headers first in required order.
	for (size_t i = 0; i < preferredCount; ++i)
	{
		std::map<std::string, std::string>::const_iterator it = headers.find(preferredOrder[i]);
		if (it != headers.end())
			oss << it->first << ": " << it->second << "\r\n";
	}

	// Emit all remaining headers (e.g. Location / CGI-specific headers).
	for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it)
	{
		bool isPreferred = false;
		for (size_t i = 0; i < preferredCount; ++i)
		{
			if (it->first == preferredOrder[i])
			{
				isPreferred = true;
				break;
			}
		}
		if (!isPreferred)
			oss << it->first << ": " << it->second << "\r\n";
	}

	oss << "\r\n";
	oss << body;
	return (oss.str());
}
