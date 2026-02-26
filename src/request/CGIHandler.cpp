#include "../../inc/request/CGIHandler.hpp"
#include "../../inc/response/ResponseBuilder.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <vector>
#include <map>
#include <string>
#include <cctype>
#include <sstream>
#include <cerrno>
#include <fcntl.h>
#include <signal.h>
#include <cstdlib>

bool CGIHandler::createCgiPipes(int stdinPipe[2], int stdoutPipe[2])
{
    if (pipe(stdinPipe) < 0)
        return false;
    if (pipe(stdoutPipe) < 0)
    {
        close(stdinPipe[0]); close(stdinPipe[1]);
        return false;
    }
    return true;
}

void CGIHandler::closeCgiPipes(int stdinPipe[2], int stdoutPipe[2])
{
    close(stdinPipe[0]); close(stdinPipe[1]);
    close(stdoutPipe[0]); close(stdoutPipe[1]);
}

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

std::vector<char*> CGIHandler::buildCgiArgv(const std::string& interpreter,
                                            const std::string& scriptPath)
{
    std::vector<char*> argv;
    argv.push_back(const_cast<char*>(interpreter.c_str()));
    argv.push_back(const_cast<char*>(scriptPath.c_str()));
    argv.push_back(NULL);
    return argv;
}

bool CGIHandler::parseCgiOutput(const std::string& raw,
                                int& statusCode,
                                std::map<std::string, std::string>& headers,
                                std::string& body)
{
    // Split CGI output into header block and body block.
    // Accept CRLF and LF-only separators for robustness.
    size_t sep = raw.find("\r\n\r\n");
    size_t sepLen = 4;
    if (sep == std::string::npos)
    {
        sep = raw.find("\n\n");
        sepLen = 2;
    }
    // Missing separator means malformed CGI response format.
    if (sep == std::string::npos)
        return false;

    // Everything before separator is headers; after separator is body.
    std::string headerBlock = raw.substr(0, sep);
    body = raw.substr(sep + sepLen);
    // Default CGI status when "Status" header is absent.
    statusCode = 200;

    // Parse headers line-by-line.
    size_t start = 0;
    while (start < headerBlock.size())
    {
        size_t end = headerBlock.find('\n', start);
        if (end == std::string::npos)
            end = headerBlock.size();

        // Trim optional CR from CRLF-formatted lines.
        std::string line = headerBlock.substr(start, end - start);
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);

        if (!line.empty())
        {
            // Each header line must be "Key: Value".
            size_t colon = line.find(':');
            if (colon == std::string::npos)
                return false;

            std::string key = line.substr(0, colon);
            std::string value = line.substr(colon + 1);
            // Remove leading whitespace from header value.
            while (!value.empty() && (value[0] == ' ' || value[0] == '\t'))
                value.erase(0, 1);

            // CGI may provide "Status: 200 OK" style status.
            // We parse the numeric status code and validate range.
            if (key == "Status")
            {
                std::istringstream iss(value);
                int parsed = 0;
                iss >> parsed;
                if (!iss || parsed < 100 || parsed > 599)
                    return false;
                statusCode = parsed;
            }
            // Preserve all headers for response construction.
            headers[key] = value;
        }

        start = end + 1;
    }

    return true;
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

RequestResult CGIHandler::startCgi(const HttpRequest& request,
                                   const Location& location,
                                   const std::string& fullPath,
                                   const ServerConfig& serverConfig)
{
    CgiStartData startData;

    // CGI should only handle GET/POST in this project scope.
    if (request.getMethod() != "GET" && request.getMethod() != "POST")
        return RequestResult::immediate(ResponseBuilder::buildErrorResponse(405, serverConfig));

    // Resolve CGI extension/interpreter from location config.
    std::string scriptExtension = extractExtension(fullPath);
    if (scriptExtension != ".py")
        return RequestResult::immediate(ResponseBuilder::buildErrorResponse(501, serverConfig));

    std::string interpreterPath = location.interpreter_path;
    if (interpreterPath.empty())
        return RequestResult::immediate(ResponseBuilder::buildErrorResponse(500, serverConfig));

    // Interpreter path must resolve to a readable/executable regular file.
    FileInfo interpreterInfo = FileService::getFileInfo(interpreterPath);
    if (interpreterInfo.status != FILE_OK)
        return RequestResult::immediate(ResponseBuilder::buildErrorResponse(500, serverConfig));
    if (!interpreterInfo.isRegularFile || !interpreterInfo.readable || !interpreterInfo.executable)
        return RequestResult::immediate(ResponseBuilder::buildErrorResponse(500, serverConfig));

    // Support URI path-info by trimming filesystem path to actual script file.
    std::string scriptPath = extractScriptPath(fullPath, scriptExtension);
    FileInfo scriptInfo = FileService::getFileInfo(scriptPath);

    // Target script must resolve to an existing regular file and be readable.
    if (scriptInfo.status == FILE_NOT_FOUND)
        return RequestResult::immediate(ResponseBuilder::buildErrorResponse(404, serverConfig));
    if (scriptInfo.status == FILE_FORBIDDEN)
        return RequestResult::immediate(ResponseBuilder::buildErrorResponse(403, serverConfig));
    if (scriptInfo.status != FILE_OK)
        return RequestResult::immediate(ResponseBuilder::buildErrorResponse(500, serverConfig));
    if (!scriptInfo.isRegularFile)
        return RequestResult::immediate(ResponseBuilder::buildErrorResponse(404, serverConfig));
    if (!scriptInfo.readable)
        return RequestResult::immediate(ResponseBuilder::buildErrorResponse(403, serverConfig));

    // CGI environment from request + server context.
    std::vector<std::string> envStorage = buildCgiEnv(request, location, serverConfig, scriptPath, scriptExtension);

    // execve envp as vector of pointers to strings in envStorage
    std::vector<char*> envp;
    for (size_t i = 0; i < envStorage.size(); ++i)
        envp.push_back(const_cast<char*>(envStorage[i].c_str()));
    envp.push_back(NULL);

    // argv for execve (interpreter + script).
    std::vector<char*> argv = buildCgiArgv(interpreterPath, scriptPath);

    // Create stdout pipe always; stdin pipe only for POST.
    const bool isPost = (request.getMethod() == "POST");
    int stdinPipe[2] = { -1, -1 };
    int stdoutPipe[2] = { -1, -1 };

    if (isPost && pipe(stdinPipe) < 0)
        return RequestResult::immediate(ResponseBuilder::buildErrorResponse(500, serverConfig));
    if (pipe(stdoutPipe) < 0)
    {
        if (isPost)
        {
            close(stdinPipe[0]);
            close(stdinPipe[1]);
        }
        return RequestResult::immediate(ResponseBuilder::buildErrorResponse(500, serverConfig));
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        if (isPost)
        {
            close(stdinPipe[0]);
            close(stdinPipe[1]);
        }
        close(stdoutPipe[0]);
        close(stdoutPipe[1]);
        return RequestResult::immediate(ResponseBuilder::buildErrorResponse(500, serverConfig));
    }

    if (pid == 0)
    {
        close(stdoutPipe[0]);

        if (isPost)
        {
            close(stdinPipe[1]);
            if (dup2(stdinPipe[0], STDIN_FILENO) < 0)
            {
                close(stdinPipe[0]);
                close(stdoutPipe[1]);
                std::exit(1);
            }
            close(stdinPipe[0]);
        }

        if (dup2(stdoutPipe[1], STDOUT_FILENO) < 0)
        {
            close(stdoutPipe[1]);
            std::exit(1);
        }
        close(stdoutPipe[1]);

        size_t slashPos = scriptPath.find_last_of('/');
        std::string scriptDir = (slashPos == std::string::npos) ? "." : scriptPath.substr(0, slashPos);
        if (scriptDir.empty())
            scriptDir = "/";
        if (chdir(scriptDir.c_str()) < 0)
            std::exit(1);

        execve(interpreterPath.c_str(), &argv[0], &envp[0]);
        std::exit(1);
    }

    // close child-side pipe ends
    if (isPost)
        close(stdinPipe[0]);
    close(stdoutPipe[1]);

    // Return parent-side descriptors/pid for deferred poll-driven CGI handling.
    startData.active = true;
    startData.pid = pid;
    startData.stdinFd = isPost ? stdinPipe[1] : -1;
    startData.stdoutFd = stdoutPipe[0];
    startData.requestBody = isPost ? request.getBody() : "";
    return RequestResult::deferredCgi(startData);
}
