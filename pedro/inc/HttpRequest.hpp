#pragma once
#include <string>
#include <map>
#include "Mime.hpp"

class HttpRequest
{
	private:
		std::string method;
		std::string uri;
		std::string version;
		std::map<std::string, std::string> headers;
		std::string body;
		static std::string sanitizeFilename(const std::string& raw);

	public:
		HttpRequest();

		// Getters
		std::string getMethod() const;
		std::string getUri() const;
		std::string getVersion() const;
		std::map<std::string, std::string> getHeaders() const;
		std::string getBody() const;

		// Setters
		void setMethod(const std::string& m);
		void setUri(const std::string& u);
		void setVersion(const std::string& v);
		void addHeader(const std::string& key, const std::string& value);
		void setBody(const std::string& b);


		// Helper methods
		bool hasHeader(const std::string& key) const;
		bool tryGetHeader(const std::string& key, std::string& value) const;
		std::string extensionFromContentType() const;
		std::string filenameFromContentDisposition()const;
};
