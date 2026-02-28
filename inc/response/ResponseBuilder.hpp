#pragma once
#include "HttpResponse.hpp"
#include "../config/ServerConfig.hpp"
#include "../internal/FileService.hpp"
#include "../response/Mime.hpp"
#include <map>

// Factory Class (stateless)
class ResponseBuilder
{
	private:
    	static int statusFromInfo(const FileInfo& info);
		static std::string escapeHtml(const std::string& input);
		static std::string getMimeType(const std::string& path);
	    static std::string loadErrorPage(int statusCode, const ServerConfig& serverConfig);
	
	public:
		static std::string resolveReasonPhrase(int statusCode);
		static HttpResponse buildErrorResponse(int statusCode, const ServerConfig& serverConfig);
		static HttpResponse buildErrorResponse(const FileInfo& info, const ServerConfig& serverConfig);
		static HttpResponse buildRedirectResponse(const Location& location);
    	static HttpResponse buildSimpleResponse(int statusCode, const std::string& body, const std::string& contentType);
    	static HttpResponse buildFileResponse(const std::string& fileContent, const std::string& filePath);
		static HttpResponse buildAutoindexResponse(const std::string& path, const std::vector<std::string>& entries);
};
