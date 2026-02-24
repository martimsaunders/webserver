#pragma once
#include "HttpRequest.hpp"
#include "../internal/FileService.hpp"
#include "../response/HttpResponse.hpp"
#include "../config/LocationConfig.hpp"
#include "../config/ServerConfig.hpp"
#include <vector>

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
    static std::string extractScriptPath(const std::string& fullPath, const std::string& ext);
    static std::string extractPathInfo(const std::string& uriPath, const std::string& ext);
    static std::string extractScriptName(const std::string& uriPath, const std::string& pathInfo);
    static std::string resolveInterpreter(const Location& location, const std::string& ext);
    static std::vector<char*> buildCgiArgv(const std::string& interpreter,
                                           const std::string& scriptPath);
    static std::vector<std::string> buildCgiEnv(const HttpRequest& request,
                                                const Location& location,
                                                const ServerConfig& serverConfig,
                                                const std::string& scriptPath,
                                                const std::string& scriptExtension);
};
