#pragma once
#include <string>
#include <map>

class HttpResponse
{
	private:
		int statusCode;
		std::string reasonPhrase;
		std::map<std::string, std::string> headers;
		std::string body;

	public:
		HttpResponse();

		// Getters
		int getStatusCode() const;
		std::string getReasonPhrase() const;
		const std::map<std::string, std::string>& getHeaders() const;
		const std::string& getBody() const;

		// Setters
		void setStatusCode(int code);
		void setReasonPhrase(const std::string &phrase);
		void addHeader(const std::string& key, const std::string& value);
		void setBody(const std::string& content);
};
