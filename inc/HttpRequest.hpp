#pragma once
#include <string>
#include <map>
#include <vector>
#include <iostream>
#define NEWLINE "\r\n"
#define HEADEND "\r\n\r\n"

class HttpRequest
{
	private:
		std::string requestBuffer;
		std::string method;
		std::string uri;
		std::string version;
		std::map<std::string, std::string> headers;
		std::string body;
		size_t bodySize;
		enum ParseStatus { Incomplete, Complete, Error };
		enum RequestState { ReadingStartLineAndHeaders, ReadingContentLengthBody, ReadingChunkedBody };
		ParseStatus status;
		RequestState state;

		// Helper methods
		std::vector<std::string> split(std::string str, const std::string& c);
		void readStartLineAndHeaders(std::string& requestBuffer);
		void readChunkedBody(std::string& requestBuffer);
		
	public:
		HttpRequest();
		
		// Getters
		std::string getMethod() const;
		std::string getUri() const;
		std::string getVersion() const;
		size_t getBodySize() const;
		
		// Setters
		void setMethod(const std::string& m);
		void setUri(const std::string& u);
		void setVersion(const std::string& v);
		void addHeader(const std::string& key, const std::string& value);
		void setBody(const std::string& b);
		void setBodySize(size_t i);
		
		// Helper methods
		bool hasHeader(const std::string& key) const;
		bool tryGetHeader(const std::string& key, std::string& value) const;
		bool hasRequest(const std::string& recvBuffer);
};
