#include "../../inc/request/CGIHandler.hpp"
#include "../../inc/response/ResponseBuilder.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <vector>
#include <map>
#include <string>
#include <cctype>
#include <sstream>

std::string CGIHandler::extractExtension(const std::string& path)
{
    size_t end = path.size();
    while (end > 0)
    {
        size_t slashPos = path.rfind('/', end - 1);
        size_t start = (slashPos == std::string::npos) ? 0 : slashPos + 1;
        std::string segment = path.substr(start, end - start);
        size_t dotPos = segment.rfind('.');
        if (dotPos != std::string::npos && dotPos > 0 && dotPos + 1 < segment.size())
            return segment.substr(dotPos);
        if (start == 0)
            break;
        end = start - 1;
    }
    return "";
}

std::string CGIHandler::extractScriptPath(const std::string& fullPath, const std::string& ext)
{
    if (ext.empty())
        return fullPath;

    size_t pos = fullPath.find(ext);
    while (pos != std::string::npos)
    {
        size_t extEnd = pos + ext.size();
        if (extEnd == fullPath.size() || fullPath[extEnd] == '/')
            return fullPath.substr(0, extEnd);
        pos = fullPath.find(ext, pos + 1);
    }
    return fullPath;
}

std::string CGIHandler::extractPathInfo(const std::string& uriPath, const std::string& ext)
{
    if (uriPath.empty() || ext.empty())
        return "";

    size_t pos = uriPath.find(ext);
    while (pos != std::string::npos)
    {
        size_t extEnd = pos + ext.size();
        if (extEnd == uriPath.size())
            return "";
        if (uriPath[extEnd] == '/')
            return uriPath.substr(extEnd);
        pos = uriPath.find(ext, pos + 1);
    }
    return "";
}

std::string CGIHandler::extractScriptName(const std::string& uriPath, const std::string& pathInfo)
{
    if (uriPath.empty() || pathInfo.empty())
        return uriPath;
    if (uriPath.size() < pathInfo.size())
        return uriPath;

    size_t splitPos = uriPath.size() - pathInfo.size();
    if (uriPath.substr(splitPos) == pathInfo)
        return uriPath.substr(0, splitPos);
    return uriPath;
}

std::string CGIHandler::resolveInterpreter(const Location& location, const std::string& ext)
{
    std::map<std::string, std::string>::const_iterator it = location.cgi.find(ext);
    if (it == location.cgi.end())
        return "";
    return it->second;
}

std::vector<char*> CGIHandler::buildCgiArgv(const std::string& interpreter,
                                            const std::string& scriptPath)
{
    std::vector<char*> argv;
    argv.push_back(const_cast<char*>(interpreter.c_str()));
    argv.push_back(const_cast<char*>(scriptPath.c_str()));
    argv.push_back(NULL);
    return argv;
}

std::vector<std::string> CGIHandler::buildCgiEnv(const HttpRequest& request,
                                                 const Location& location,
                                                 const ServerConfig& serverConfig,
                                                 const std::string& scriptPath,
                                                 const std::string& scriptExtension)
{
    (void)location;
    std::vector<std::string> env;
    const std::string pathInfo = extractPathInfo(request.getUriPath(), scriptExtension);
    const std::string scriptName = extractScriptName(request.getUriPath(), pathInfo);

    env.push_back("REQUEST_METHOD=" + request.getMethod());
    env.push_back("QUERY_STRING=" + request.getQueryString());
    env.push_back("SERVER_PROTOCOL=" + request.getVersion());
    env.push_back("SCRIPT_FILENAME=" + scriptPath);
    env.push_back("SCRIPT_NAME=" + scriptName);
    env.push_back("PATH_INFO=" + pathInfo);
    env.push_back("SERVER_SOFTWARE=webserv/1.0");
    env.push_back("SERVER_NAME=" + serverConfig.host);
    std::ostringstream portStream;
    portStream << serverConfig.port;
    env.push_back("SERVER_PORT=" + portStream.str());

    std::string contentType;
    if (request.tryGetHeader("content-type", contentType) || request.tryGetHeader("content-type:", contentType))
        env.push_back("CONTENT_TYPE=" + contentType);

    std::string contentLength;
    if (request.tryGetHeader("content-length", contentLength) || request.tryGetHeader("content-length:", contentLength))
        env.push_back("CONTENT_LENGTH=" + contentLength);

    if (!request.getHost().empty())
        env.push_back("HTTP_HOST=" + request.getHost());

    const std::map<std::string, std::string>& headers = request.getHeaders();
    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it)
    {
        std::string key = it->first;
        while (!key.empty() && key[key.size() - 1] == ':')
            key.erase(key.size() - 1);

        for (size_t i = 0; i < key.size(); ++i)
        {
            if (key[i] == '-')
                key[i] = '_';
            else
                key[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(key[i])));
        }

        if (key == "CONTENT_TYPE" || key == "CONTENT_LENGTH" || key == "HOST")
            continue;
        env.push_back("HTTP_" + key + "=" + it->second);
    }

    return env;
}

HttpResponse CGIHandler::execute(const HttpRequest& request,
                                 const Location& location,
                                 const std::string& fullPath,
                                 const FileInfo& info,
                                 const ServerConfig& serverConfig)
{
    (void)info;

    // CGI should only handle GET/POST in this project scope.
    if (request.getMethod() != "GET" && request.getMethod() != "POST")
        return ResponseBuilder::buildErrorResponse(405, serverConfig);

    // Resolve CGI extension/interpreter from location config.
    std::string scriptExtension = extractExtension(fullPath);
    std::string interpreterPath = resolveInterpreter(location, scriptExtension);
    if (scriptExtension.empty() || interpreterPath.empty())
        return ResponseBuilder::buildErrorResponse(500, serverConfig);

    // Support URI path-info by trimming filesystem path to actual script file.
    std::string scriptPath = extractScriptPath(fullPath, scriptExtension);
    FileInfo scriptInfo = FileService::getFileInfo(scriptPath);

    // Target script must resolve to an existing regular file.
	if (scriptInfo.status == FILE_NOT_FOUND)
        return ResponseBuilder::buildErrorResponse(404, serverConfig);
    if (scriptInfo.status == FILE_FORBIDDEN)
        return ResponseBuilder::buildErrorResponse(403, serverConfig);
	if (scriptInfo.status != FILE_OK)
        return ResponseBuilder::buildErrorResponse(500, serverConfig);
    if (!scriptInfo.isRegularFile)
		return ResponseBuilder::buildErrorResponse(404, serverConfig);

    // Validate interpreter/script execute/read permissions.
	if (!scriptInfo.readable)
		return ResponseBuilder::buildErrorResponse(403, serverConfig);
    if (access(interpreterPath.c_str(), X_OK) != 0)
        return ResponseBuilder::buildErrorResponse(500, serverConfig);

    // Build CGI environment from request + server context.
    std::vector<std::string> envStorage = buildCgiEnv(request, location, serverConfig, scriptPath, scriptExtension);
    // Create execve envp as vector of pointers to strings in envStorage
	std::vector<char*> envp;
    for (size_t i = 0; i < envStorage.size(); ++i)
        envp.push_back(const_cast<char*>(envStorage[i].c_str()));
    envp.push_back(NULL);

    // Build argv for execve (interpreter + script).
    std::vector<char*> argv = buildCgiArgv(interpreterPath, scriptPath);

    // 7) Create stdin/stdout pipes for CGI child process.
    int stdinPipe[2];
    int stdoutPipe[2];
    if (pipe(stdinPipe) < 0 || pipe(stdoutPipe) < 0)
        return ResponseBuilder::buildErrorResponse(500, serverConfig);

    pid_t pid = fork();
    if (pid < 0)
    {
        close(stdinPipe[0]); close(stdinPipe[1]);
        close(stdoutPipe[0]); close(stdoutPipe[1]);
        return ResponseBuilder::buildErrorResponse(500, serverConfig);
    }

    if (pid == 0)
    {
        // Child:
        // - dup2 stdin/stdout pipes
        // - close unused fds
        // - chdir to script directory (for relative file access)
        // - execve interpreter/script with envp
        //
        // TODO(helper): std::string scriptDir(const std::string& fullPath);
        // TODO(helper): std::string scriptName(const std::string& fullPath);
        // TODO(helper): void closePipesForChild(...);
        //
        // If any step fails, _exit(1).
        (void)request;
        (void)location;
        (void)envStorage;
        (void)envp;
        (void)argv;
        _exit(1);
    }

    // Parent:
    // - close child-side pipe ends
    // - if POST, write raw request body to CGI stdin
    // - read CGI stdout fully
    // - waitpid child and validate exit status
    //
    // TODO(helper): bool writeAll(int fd, const std::string& data);
    // TODO(helper): bool readAll(int fd, std::string& out);
    // TODO(helper): bool waitChildOk(pid_t pid, int& status);
    close(stdinPipe[0]);
    close(stdoutPipe[1]);

    // For POST, CGI input should be the raw request body.
    if (request.getMethod() == "POST")
    {
        // TODO: write request.getBody() to stdinPipe[1]
    }
    close(stdinPipe[1]);

    std::string cgiRawOutput;
    // TODO: read from stdoutPipe[0] into cgiRawOutput
    close(stdoutPipe[0]);

    int status = 0;
    if (waitpid(pid, &status, 0) < 0)
        return ResponseBuilder::buildErrorResponse(500, serverConfig);

    // 8) Parse CGI output into HTTP response:
    // Expected CGI output format:
    // Header: value\r\n
    // ...\r\n
    // \r\n
    // body
    //
    // TODO(helper): bool parseCgiOutput(const std::string& raw, int& statusCode, std::map<std::string,std::string>& headers, std::string& body);
    // If malformed output, return 502 Bad Gateway (or 500 if you prefer simpler policy).
    if (cgiRawOutput.empty())
        return ResponseBuilder::buildErrorResponse(502, serverConfig);

    // 9) Build final response from parsed CGI output.
    // TODO: construct HttpResponse with parsed status/header/body.
    return ResponseBuilder::buildErrorResponse(500, serverConfig);
}
