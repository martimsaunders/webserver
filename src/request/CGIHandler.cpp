#include "../../inc/request/CGIHandler.hpp"
#include "../../inc/response/ResponseBuilder.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <vector>
#include <map>
#include <string>

// what about path_info?
std::string CGIHandler::extractExtension(const std::string& path)
{
    size_t slashPos = path.find_last_of('/');
    size_t dotPos = path.find_last_of('.');
    if (dotPos == std::string::npos)
        return "";
    if (slashPos != std::string::npos && dotPos < slashPos)
        return "";
    return path.substr(dotPos);
}

std::string CGIHandler::resolveInterpreter(const Location& location, const std::string& ext)
{
    std::map<std::string, std::string>::const_iterator it = location.cgi.find(ext);
    if (it == location.cgi.end())
        return "";
    return it->second;
}

HttpResponse CGIHandler::execute(const HttpRequest& request,
                                 const Location& location,
                                 const std::string& fullPath,
                                 const FileInfo& info,
                                 const ServerConfig& serverConfig)
{
    // CGI should only handle GET/POST in this project scope.
    if (request.getMethod() != "GET" && request.getMethod() != "POST")
        return ResponseBuilder::buildErrorResponse(405, serverConfig);

    // Target script must resolve to an existing regular file.
	if (info.status != FILE_OK)
        return ResponseBuilder::buildErrorResponse(500, serverConfig);
    if (!info.isRegularFile)
		return ResponseBuilder::buildErrorResponse(404, serverConfig);

    // Resolve CGI extension/interpreter from location config.
    std::string scriptExtension = extractExtension(fullPath);
    std::string interpreterPath = resolveInterpreter(location, scriptExtension);

    if (scriptExtension.empty() || interpreterPath.empty())
        return ResponseBuilder::buildErrorResponse(500, serverConfig);

    // Validate interpreter/script execute/read permissions.
	if (!info.readable)
		return ResponseBuilder::buildErrorResponse(403, serverConfig);
    if (access(interpreterPath.c_str(), X_OK) != 0)
        return ResponseBuilder::buildErrorResponse(500, serverConfig);

    // 5) Build CGI environment from request + server context.
    // Required env keys typically include:
    // REQUEST_METHOD, QUERY_STRING, CONTENT_TYPE, CONTENT_LENGTH,
    // SCRIPT_NAME, PATH_INFO, SERVER_PROTOCOL, HTTP_HOST.
    // TODO(helper): std::vector<std::string> buildCgiEnv(const HttpRequest&, const Location&, const ServerConfig&, const std::string& fullPath);
    std::vector<std::string> envStorage;
    std::vector<char*> envp;

    // 6) Build argv for execve (interpreter + script).
    // TODO(helper): std::vector<char*> buildCgiArgv(const std::string& interpreter, const std::string& scriptPath);
    std::vector<char*> argv;

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
