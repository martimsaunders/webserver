#pragma once
#include "../response/Mime.hpp"
#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <algorithm>
#define NEWLINE "\r\n"
#define HEADEND "\r\n\r\n"

class HttpRequest
{
	public:
		HttpRequest();
		enum ParseStatus { Incomplete, Complete, Error, Parsing };
		enum RequestState { ReadingStartLine, ReadingHeaders, ReadingContentLengthBody, ReadingChunkedBody };
		
		// Getters
		std::string getMethod() const;
		std::string getUri() const;
		std::string getVersion() const;
		std::string getBody() const;
		ParseStatus getStatus() const;
		int getStatusCode() const;
		int getRequestSize() const;

		// Setters
		void setMethod(const std::string& m);
		void setUri(const std::string& u);
		void setVersion(const std::string& v);
		void addHeader(const std::string& key, const std::string& value);
		void setBody(const std::string& b);
		
		// Helper methods
		bool hasHeader(const std::string& key) const;
		bool tryGetHeader(const std::string& key, std::string& value) const;
		void parseRequest(const std::string& recvBuffer, size_t bodyMaxSize);
		std::string extensionFromContentType() const;
		std::string filenameFromContentDisposition()const;
		void printRequest();

	private:
		std::string method;
		std::string uri;
		std::string version;
		std::map<std::string, std::string> headers;
		std::string body;
		std::string errorMsg;
		int statusCode;
		int requestSize;
		ParseStatus status;
		RequestState state;
		static std::string sanitizeFilename(const std::string& raw);

		// Helper methods
		std::vector<std::string> split(std::string str, const std::string& c);
		int readStartLine(std::string& requestBuffer);
		int readHeaders(std::string& requestBuffer);
		int readChunkedBody(std::string& requestBuffe, size_t bodyMaxSize);
		int readContentLengthBody(std::string& requestBuffe, size_t bodyMaxSize);
};
