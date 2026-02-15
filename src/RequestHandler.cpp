// RequestHandler.cpp
#include "RequestHandler.hpp"
#include "ErrorResponseBuilder.hpp"
#include "FileService.hpp"
#include "CGIHandler.hpp"

enum HttpMethod
{
    GET,
    POST,
    DELETE,
    UNKNOWN
};

// Helper function to convert string to enum
HttpMethod stringToMethod(const std::string& method)
{
    if (method == "GET") return GET;
    if (method == "POST") return POST;
    if (method == "DELETE") return DELETE;
    return UNKNOWN;
}

HttpResponse RequestHandler::handleRequest(const HttpRequest& request, const ServerConfig& serverConfig)
{
    // Find matching Location block
    const Location* location = serverConfig.findLocation(request.getUri());
    if (!location)
        return ErrorResponseBuilder::build(404, serverConfig);

    // Validate HTTP method
    HttpMethod method = stringToMethod(request.getMethod());
    if (!location->isMethodAllowed(request.getMethod()) || method == UNKNOWN)
        return ErrorResponseBuilder::build(405, serverConfig);

    // Resolve full filesystem path
    std::string fullPath = location->resolvePath(request.getUri());
    if (!FileService::exists(fullPath))
        return ErrorResponseBuilder::build(404, serverConfig);

    // Handle redirect
    if (location->needsRedirect())
        return HttpResponse::buildRedirect(location->getRedirect());

    // Dispatch using switch
    HttpResponse response;
    switch (method)
    {
        case GET:
            // FileService read file / directory / autoindex
            break;
        case POST:
            // Handle file upload or CGI if required
            break;
        case DELETE:
            // FileService delete file
            break;
        default:
            return ErrorResponseBuilder::build(405, serverConfig);
    }

    // Execute CGI if required
    if (location->isCgiRequest(request.getUri()))
    {
        // call CGIHandler to execute script and return response
    }

    // Validate constraints (max body size, permissions, etc.)
    // if violation → return ErrorResponseBuilder

    // Fill headers, status code, and body

    // Return response to HTTP layer
    return response;
}
