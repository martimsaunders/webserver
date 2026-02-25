/*
TODO before cgi:
path parsing coming from http (split query string, invalid chars(400), traversal(400))
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

RequestResult RequestHandler::handleRequest(const HttpRequest& request, const ServerConfig& serverConfig)
{
	//reject if location block does not exist (404)
    const Location* location = serverConfig.findLocation(request.getUri());
    if (!location)
		return RequestResult::immediate(ResponseBuilder::buildErrorResponse(404, serverConfig));

	//reject if method not allowed (501)
    HttpMethod method = stringToMethod(request.getMethod());
    if (method == UNKNOWN){
        return RequestResult::immediate(ResponseBuilder::buildErrorResponse(501, serverConfig));}

	if (!checkMethod(method, *location)){
		return RequestResult::immediate(ResponseBuilder::buildErrorResponse(405, serverConfig));}

	//override with redirect if in location block (dynamic code)
    if (location->hasRedirect()){
        return RequestResult::immediate(ResponseBuilder::buildRedirectResponse(*location));}

	//reject unsafe path string (403)
    std::string fullPath = location->resolvePath(request.getUri());
    if (fullPath.empty()){
        return RequestResult::immediate(ResponseBuilder::buildErrorResponse(403, serverConfig));}

	//reject if not found (404) or forbidden(403), default to (500)
    FileInfo info = FileService::getFileInfo(fullPath); 
    if (info.status != FILE_OK){
        return RequestResult::immediate(ResponseBuilder::buildErrorResponse(info, serverConfig));}

    // Override if CGI endpoint
    if (location->is_cgi)
        return CGIHandler::startCgi(request, *location, fullPath, info, serverConfig);

    switch (method)
    {
		//implemented methods
        case GET:
            return RequestResult::immediate(handleGet(location, fullPath, info, serverConfig));
        case POST:
            return RequestResult::immediate(handlePost(location, request, fullPath, info, serverConfig));
        case DELETE:
            return RequestResult::immediate(handleDelete(location, fullPath, info, serverConfig));
        case UNKNOWN:
            return RequestResult::immediate(ResponseBuilder::buildErrorResponse(501, serverConfig));
        default:
            return RequestResult::immediate(ResponseBuilder::buildErrorResponse(500, serverConfig));
    }
}

HttpResponse RequestHandler::handleGet(const Location* location,
                                       const std::string& path,
                                       const FileInfo& info,
                                       const ServerConfig& serverConfig)
{
    if (info.isDirectory)
    {
        // Index candidates precedence: location -> server -> hardcoded default.
        std::vector<std::string> indexCandidates;
        if (!location->index.empty())
            indexCandidates.push_back(location->index);
        if (!serverConfig.index.empty() && serverConfig.index != location->index)
            indexCandidates.push_back(serverConfig.index);
        if (indexCandidates.empty() || indexCandidates.back() != "index.html")
            indexCandidates.push_back("index.html");

        // Search candidates in the resolved request directory.
        for (std::vector<std::string>::const_iterator it = indexCandidates.begin();
             it != indexCandidates.end(); ++it)
        {
            std::string indexPath = joinPath(path, *it);
            FileInfo indexInfo = FileService::getFileInfo(indexPath);

            if (indexInfo.status == FILE_NOT_FOUND)
                continue;
            if (indexInfo.status == FILE_FORBIDDEN)
                return ResponseBuilder::buildErrorResponse(403, serverConfig);
            if (indexInfo.status != FILE_OK)
                return ResponseBuilder::buildErrorResponse(indexInfo, serverConfig);
            if (!indexInfo.isRegularFile)
                continue;

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
    if (!location->upload_enabled){
        return ResponseBuilder::buildErrorResponse(403, serverConfig);}

	//upload path not configured, internal error (500)
    std::string uploadPath = location->upload_store;
    if (uploadPath.empty()){
        return ResponseBuilder::buildErrorResponse(500, serverConfig);}

	//if upload path returns error, server misconfigured (500)
    FileInfo uploadDirInfo = FileService::getFileInfo(uploadPath);
    if (uploadDirInfo.status != FILE_OK || !uploadDirInfo.isDirectory || !uploadDirInfo.writable){
        return ResponseBuilder::buildErrorResponse(500, serverConfig);}

	//if uri is not directory
	if (!info.isDirectory){
		return ResponseBuilder::buildErrorResponse(409, serverConfig);}

	//find filename or build (to create full path)
    const MultipartFile &multipartFile = request.getMultipartFile();
    std::string filename = request.filenameFromContentDisposition();
    std::string bodyToWrite = request.getBody();
    if (!multipartFile.filename.empty())
    {
        filename = multipartFile.filename;
        bodyToWrite = multipartFile.data;
    }

    std::string extension;
    if (!filename.empty() && filename.find('.') != std::string::npos)
        extension = filename.substr(filename.find_last_of('.'));
    else
    {
        if (!multipartFile.contentType.empty())
            extension = Mime::getExtension(multipartFile.contentType);
        else
            extension = request.extensionFromContentType();

        if (extension.empty())
            return ResponseBuilder::buildErrorResponse(415, serverConfig);
    }

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
    {
        targetPath = buildDefaultFilename(uploadPath, extension);
        size_t slashPos = targetPath.find_last_of('/');
        if (slashPos == std::string::npos)
            filename = targetPath;
        else
            filename = targetPath.substr(slashPos + 1);
    }

	// avoid overwriting an existing file
	if (FileService::pathExists(targetPath)){
		return ResponseBuilder::buildErrorResponse(409, serverConfig);}

	//if write fails, return internal error (500)
    if (!FileService::writeFile(targetPath, bodyToWrite)){
        return ResponseBuilder::buildErrorResponse(500, serverConfig);}

	//return file created succesfully (201)
    return ResponseBuilder::buildSimpleResponse(201, "File uploaded successfully: " + targetPath, "text/plain");
}

HttpResponse RequestHandler::handleDelete(const Location* location,
                                          const std::string& path,
                                          const FileInfo& info,
                                          const ServerConfig& serverConfig)
{
	(void) location;
	(void) path;

	// recursive deleting directories is not supported
	if (info.isDirectory){
		return ResponseBuilder::buildErrorResponse(409, serverConfig);}

	// if not writable no permission (403)
    if (!info.writable){
        return ResponseBuilder::buildErrorResponse(403, serverConfig);}
	
	// Fallback for other runtime failures (EISDIR, I/O issues, etc.)
	if (!FileService::deleteFile(path)){
    	return ResponseBuilder::buildErrorResponse(500, serverConfig);}

	return ResponseBuilder::buildSimpleResponse(200, "Deleted successfully", "text/plain");    
}
