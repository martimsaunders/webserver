#pragma once
#include "HttpRequest.hpp"
#include "../internal/FileService.hpp"
#include "../response/HttpResponse.hpp"
#include "../config/LocationConfig.hpp"
#include "../config/ServerConfig.hpp"
#include "RequestResult.hpp"
#include <vector>
#include <map>
#include <sys/types.h>

class CGIHandler
{
public:
    static RequestResult startCgi(
        const HttpRequest& request,
        const Location& location,
        const std::string& fullPath,
        const ServerConfig& serverConfig
    );

private:
    static bool createCgiPipes(int stdinPipe[2], int stdoutPipe[2]);
    static void closeCgiPipes(int stdinPipe[2], int stdoutPipe[2]);
    static std::string extractExtension(const std::string& path);
    static std::string extractScriptPath(const std::string& fullPath, const std::string& ext);
    static std::string extractPathInfo(const std::string& uriPath, const std::string& ext);
    static std::string extractScriptName(const std::string& uriPath, const std::string& pathInfo);
    static std::vector<char*> buildCgiArgv(const std::string& interpreter,
                                           const std::string& scriptPath);
    static bool parseCgiOutput(const std::string& raw,
                               int& statusCode,
                               std::map<std::string, std::string>& headers,
                               std::string& body);
    static std::vector<std::string> buildCgiEnv(const HttpRequest& request,
                                                const Location& location,
                                                const ServerConfig& serverConfig,
                                                const std::string& scriptPath,
                                                const std::string& scriptExtension);
};
