#pragma once
#include <string>
#include <map>

class HttpRequest
{
	private:
		std::string method;
		std::string uri;
		std::string version;
		std::map<std::string, std::string> headers;
		std::string body;

	public:
		HttpRequest();

		// Helper methods
		std::string getMethod() const;
		std::string getUri() const;
		std::string getVersion() const;
		bool hasHeader(const std::string& key) const;
		std::string getHeader(const std::string& key) const;
		bool matchesCgi(const std::string& extension) const;
};
