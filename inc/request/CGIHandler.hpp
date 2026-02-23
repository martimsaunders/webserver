#pragma once
#include "HttpRequest.hpp"
#include "../internal/FileService.hpp"
#include "../response/HttpResponse.hpp"
#include "../config/LocationConfig.hpp"
#include "../config/ServerConfig.hpp"

class CGIHandler
{
public:
    static HttpResponse execute(
        const HttpRequest& request,
        const Location& location,
        const std::string& fullPath,
        const FileInfo& info,
        const ServerConfig& serverConfig
    );

private:
    static std::string extractExtension(const std::string& path);
    static std::string resolveInterpreter(const Location& location, const std::string& ext);
};
