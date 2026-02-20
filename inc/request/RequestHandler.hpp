#pragma once
#include "../internal/FileService.hpp"
#include "HttpRequest.hpp"
#include "../response/HttpResponse.hpp"
#include "../config/ServerConfig.hpp"

// Router + Controller Class (stateless)
class RequestHandler
{
	private:
			static bool checkMethod(int method, Location location);
			static std::string buildDefaultFilename(const std::string& directory, const std::string& extension);
			static std::string joinPath(const std::string& directory, const std::string& filename);
	public:
		static HttpResponse handleRequest(
			const HttpRequest& request,
			const ServerConfig& server
		);
		static HttpResponse handleGet(const Location* location,
            const std::string& path,
            const FileInfo& info,
            const ServerConfig& serverConfig
		);
		static HttpResponse handlePost(const Location* location,
			const HttpRequest& request,
			const std::string& path,
			const FileInfo& info,
			const ServerConfig& serverConfig
		);
		static HttpResponse handleDelete(const Location* location,
			const std::string& path,
			const FileInfo& info,
			const ServerConfig& serverConfig
		);
};
