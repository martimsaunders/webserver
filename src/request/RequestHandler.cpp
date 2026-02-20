/*
TODO:
need headers trimmed and in lowercase from parsing - DONE
is serverconfig index file a name or path? - DONE
introduce allowed methods in post/delete - DONE
post only with uploadpath or uri sensitive (at least !eisdir) - DONE
status code visual map 
path parsing coming from http (igonre query strings, ...) 
introduce response constraints (maxlength, ...)
*/

#include "../../inc/request/RequestHandler.hpp"
#include "../../inc/request/CGIHandler.hpp"
#include "../../inc/response/ResponseBuilder.hpp"
#include "../../inc/internal/FileService.hpp"
#include <cctype>
#include <ctime>
#include <sstream>

enum HttpMethod
{
    GET,
    POST,
    DELETE,
    UNKNOWN
};

// Helper functions
HttpMethod stringToMethod(const std::string& method)
{
    if (method == "GET") return GET;
    if (method == "POST") return POST;
    if (method == "DELETE") return DELETE;
    return UNKNOWN;
}

bool RequestHandler::checkMethod(int method, Location location)
{
	switch (method)
	{
		case GET:
			return location.allow_get;
		case POST:
			return location.allow_post;
		case DELETE:
			return location.allow_delete;
	}
	return false;
}

std::string RequestHandler::buildDefaultFilename(const std::string& directory, const std::string& extension)
{
    static unsigned long uploadCounter = 0;
    ++uploadCounter;

    std::ostringstream filename;
    filename << "upload_" << static_cast<unsigned long>(std::time(NULL))
             << "_" << uploadCounter << extension;

    if (!directory.empty() && directory[directory.size() - 1] == '/')
        return directory + filename.str();
    return directory + "/" + filename.str();
}

std::string RequestHandler::joinPath(const std::string& directory, const std::string& filename)
{
    if (!directory.empty() && directory[directory.size() - 1] == '/')
        return directory + filename;
    return directory + "/" + filename;
}

HttpResponse RequestHandler::handleRequest(const HttpRequest& request, const ServerConfig& serverConfig)
{
	//reject if location block does not exist (404)
    const Location* location = serverConfig.findLocation(request.getUri());
    if (!location)
		return ResponseBuilder::buildErrorResponse(404, serverConfig);

	//reject if method not allowed (405)
    HttpMethod method = stringToMethod(request.getMethod());
    if (method == UNKNOWN)
        return ResponseBuilder::buildErrorResponse(405, serverConfig);
	if (!checkMethod(method, *location))
		return ResponseBuilder::buildErrorResponse(405, serverConfig);

	//override with redirect if in location block (302)
    if (location->hasRedirect())
        return ResponseBuilder::buildRedirectResponse(*location);

	//reject unsafe path string (403)
    std::string fullPath = location->resolvePath(request.getUri());
    if (fullPath.empty())
        return ResponseBuilder::buildErrorResponse(403, serverConfig);

	//reject if not found (404) or forbidden(403), default to (500)
    FileInfo info = FileService::getFileInfo(fullPath); 
    if (info.status != FILE_OK)
        return ResponseBuilder::buildErrorResponse(info, serverConfig);

    HttpResponse response;
    switch (method)
    {
		//reject if invalid method (405)
        case GET:
            response = handleGet(location, fullPath, info, serverConfig);
            break;
        case POST:
            response = handlePost(location, request, fullPath, info, serverConfig);
            break;
        case DELETE:
            response = handleDelete(location, fullPath, info, serverConfig);
            break;
        default:
            return ResponseBuilder::buildErrorResponse(405, serverConfig);
    }

    // Execute CGI if required
    if (location->isCgiRequest(request.getUri()))
    {
        // call CGIHandler to execute script and return response
    }

    // Validate constraints (max body size, permissions, etc.)
    // if violation → return ErrorResponseBuilder
    return response;
}

HttpResponse RequestHandler::handleGet(const Location* location,
                                       const std::string& path,
                                       const FileInfo& info,
                                       const ServerConfig& serverConfig)
{
    if (info.isDirectory)
    {
        // Index precedence: location -> server -> hardcoded default.
		std::string indexFile = location->index;
		if (indexFile.empty())
			indexFile = serverConfig.index;
		if (indexFile.empty())
    		indexFile = "index.html";

		// Always search index in the resolved request directory.
		std::string indexPath = joinPath(path, indexFile);
		FileInfo indexInfo = FileService::getFileInfo(indexPath);
		if (indexInfo.status == FILE_OK && indexInfo.isRegularFile)
		{
			std::string body;
			if (!FileService::readFile(indexPath, body))
				return ResponseBuilder::buildErrorResponse(403, serverConfig);
			return ResponseBuilder::buildFileResponse(body, indexPath);
		}

		// No usable index -> optional autoindex.
		if (location->autoindex)
		{
			std::vector<std::string> entries;
			if (!FileService::listDirectory(path, entries))
				return ResponseBuilder::buildErrorResponse(403, serverConfig);
			return ResponseBuilder::buildAutoindexResponse(path, entries);
		}
		//directory exists but listing disabled, no autoindex. forbidden (403)
        else
            return ResponseBuilder::buildErrorResponse(403, serverConfig);
    }

    // Regular file
    std::string body;

	// reject if file not readable. Forbidden (403)
    if (!FileService::readFile(path, body))
        return ResponseBuilder::buildErrorResponse(403, serverConfig);

	//return file (200)
    return ResponseBuilder::buildFileResponse(body, path);
}


HttpResponse RequestHandler::handlePost(const Location* location,
                                        const HttpRequest& request,
                                        const std::string& path,
                                        const FileInfo& info,
                                        const ServerConfig& serverConfig)
{
	(void) path;
    (void) info;

	// reject if upload disabled, Forbidden (403)
    if (!location->upload_enabled)
        return ResponseBuilder::buildErrorResponse(403, serverConfig);

	//upload path not configured, internal error (500)
    std::string uploadPath = location->upload_store;
    if (uploadPath.empty())
        return ResponseBuilder::buildErrorResponse(500, serverConfig);

	//if upload path returns error, server misconfigured (500)
    FileInfo uploadDirInfo = FileService::getFileInfo(uploadPath);
    if (uploadDirInfo.status != FILE_OK || !uploadDirInfo.isDirectory || !uploadDirInfo.writable)
        return ResponseBuilder::buildErrorResponse(500, serverConfig);

	//if uri is not directory
	if (!info.isDirectory)
		return ResponseBuilder::buildErrorResponse(404, serverConfig);

	//find filename or build (to create full path)
    std::string extension = request.extensionFromContentType();
    std::string filename = request.filenameFromContentDisposition();
    std::string targetPath;
	
	//filename found (path + filename)
    if (!filename.empty())
    {
        if (filename.find('.') == std::string::npos)
            filename += extension;
        targetPath = joinPath(uploadPath, filename);
    }
	//if no filename, build defaultName (path + defaultName)
    else
        targetPath = buildDefaultFilename(uploadPath, extension);

	//if write fails, return internal error (500)
    if (!FileService::writeFile(targetPath, request.getBody()))
        return ResponseBuilder::buildErrorResponse(500, serverConfig);

	//return file created succesfully (201)
    return ResponseBuilder::buildSimpleResponse(201, "File uploaded successfully: " + targetPath, "text/plain");
}

HttpResponse RequestHandler::handleDelete(const Location* location,
                                          const std::string& path,
                                          const FileInfo& info,
                                          const ServerConfig& serverConfig)
{
	(void) location;

	// if not writable no permission (403)
    if (!info.writable)
        return ResponseBuilder::buildErrorResponse(403, serverConfig);
	
	// Fallback for other runtime failures (EISDIR, I/O issues, etc.)
	if (!FileService::deleteFile(path))
    	return ResponseBuilder::buildErrorResponse(500, serverConfig);

	return ResponseBuilder::buildSimpleResponse(200, "Deleted successfully", "text/plain");    
}
