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

struct MultipartFile
{
	std::string filename;
	std::string contentType;
	std::string data;
};

class HttpRequest
{
	public:
		HttpRequest();
		enum ParseStatus { Incomplete, Complete, Error, Parsing };
		enum RequestState { ReadingStartLine, ReadingHeaders, ReadingContentLengthBody, ReadingChunkedBody };
		
		// Getters
		std::string getMethod() const;
		std::string getUri() const;
		std::string getUriPath() const;
		std::string getQueryString() const;
		std::string getHost() const;
		std::string getVersion() const;
		std::string getBody() const;
		ParseStatus getStatus() const;
		int getStatusCode() const;
		int getRequestSize() const;
		const MultipartFile& getMultipartFile() const;
		bool getIsMultipart() const;
		const std::string& getBoundary() const;

		// Setters
		void setMethod(const std::string& m);
		void setUri(const std::string& u);
		void setUriPath(const std::string& p);
		void setQueryString(const std::string& q);
		void setHost(const std::string& h);
		void setVersion(const std::string& v);
		void addHeader(const std::string& key, const std::string& value);
		void setBody(const std::string& b);
		void setIsMultipart(bool value);
		void setBoundary(const std::string& value);
		void setMultipartFile(const MultipartFile& value);
		
		// Helper methods
		bool hasHeader(const std::string& key) const;
		bool tryGetHeader(const std::string& key, std::string& value) const;
		void parseRequest(const std::string& recvBuffer, size_t bodyMaxSize);
		std::string extensionFromContentType() const;
		std::string filenameFromContentDisposition()const;
		bool detectMultipartAndBoundary(); // Detect multipart/form-data and extract the boundary token.
		int extractFirstFilePartToBody(); // Extract first multipart file part and normalize related headers.
		void printRequest();

	private:
		std::string method;
		std::string uri;
		// URI path component without query string, used for CGI SCRIPT_NAME/PATH_INFO logic.
		std::string uriPath;
		// URI query component (characters after '?'), used for CGI QUERY_STRING.
		std::string queryString;
		// Host header value from request, used for CGI HTTP_HOST.
		std::string host;
		std::string version;
		std::map<std::string, std::string> headers;
		std::string body;
		std::string errorMsg;
		int statusCode;
		int requestSize;
		ParseStatus status;
		RequestState state;
		bool isMultipart; // request Content-Type is multipart/form-data.
		std::string boundary; //Multipart boundary token extracted from Content-Type.
		MultipartFile file; //First extracted multipart file part (filename/content-type/data).

		// Helper methods
		static std::string sanitizeFilename(const std::string& raw);
		std::vector<std::string> split(std::string str, const std::string& c);
		int readStartLine(std::string& requestBuffer);
		int readHeaders(std::string& requestBuffer);
		int readChunkedBody(std::string& requestBuffe, size_t bodyMaxSize);
		int readContentLengthBody(std::string& requestBuffe, size_t bodyMaxSize);
		int multipartParsing();
};


/*
400 cases multipart:
- multipart without boundary
- empty/invalid boundary
- missing boundary delimiters in body
- malformed multipart structure
- no file part (filename=)
- malformed file part Content-Disposition
- empty extracted file data
*/