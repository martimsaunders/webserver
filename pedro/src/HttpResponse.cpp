#include "../inc/HttpResponse.hpp"

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
