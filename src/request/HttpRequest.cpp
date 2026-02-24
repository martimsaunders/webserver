#include "../../inc/request/HttpRequest.hpp"

HttpRequest::HttpRequest() : method(""), uri(""), uriPath(""), queryString(""), host(""), version(""), headers(), body(""), errorMsg(""), 
statusCode(0), requestSize(0), status(Parsing), state(ReadingStartLine),
isMultipart(false), boundary(""), file() {}
	
// Getters
std::string HttpRequest::getMethod() const { return method; }
std::string HttpRequest::getUri() const { return uri; }
std::string HttpRequest::getUriPath() const { return uriPath; }
std::string HttpRequest::getQueryString() const { return queryString; }
std::string HttpRequest::getHost() const { return host; }
std::string HttpRequest::getVersion() const { return version; }
std::string HttpRequest::getBody() const { return body; }
HttpRequest::ParseStatus HttpRequest::getStatus() const { return status; }
int HttpRequest::getStatusCode() const { return statusCode; }
int HttpRequest::getRequestSize() const { return requestSize; }
const MultipartFile& HttpRequest::getMultipartFile() const { return file; }
bool HttpRequest::getIsMultipart() const { return isMultipart; }
const std::string& HttpRequest::getBoundary() const { return boundary; }

//Setters
void HttpRequest::setMethod(const std::string& m) { method = m; }
void HttpRequest::setUri(const std::string& u) { uri = u; }
void HttpRequest::setUriPath(const std::string& p) { uriPath = p; }
void HttpRequest::setQueryString(const std::string& q) { queryString = q; }
void HttpRequest::setHost(const std::string& h) { host = h; }
void HttpRequest::setVersion(const std::string& v) { version = v; }
void HttpRequest::addHeader(const std::string& key, const std::string& value) {  headers[key] = value; }
void HttpRequest::setBody(const std::string& b) { body = b; }
void HttpRequest::setIsMultipart(bool value) { isMultipart = value; }
void HttpRequest::setBoundary(const std::string& value) { boundary = value; }
void HttpRequest::setMultipartFile(const MultipartFile& value) { file = value; }

int HttpRequest::extractFirstFilePartToBody(std::vector<std::string> multipart){
    
    std::string filePart;
    std::string key;
    for(std::vector<std::string>::iterator it = multipart.begin(); it != multipart.end(); it++){
        
        key = "name=\"file\"";
        if((*it).find(key) == std::string::npos)
            continue;
        
        filePart = *it;
        break;
    }
    if(filePart.empty()){
        errorMsg = "Multipart body: Missing file part";
        return 400;
    }
    //file name
    key = "filename=\"";
    size_t start = filePart.find(key);

    if(start == std::string::npos){
        errorMsg = "Multipart body: Missing file name";
        return 400;
    }
    start += key.size();

    size_t end = filePart.find("\"", start);
    if(end == std::string::npos){
        errorMsg = "Multipart body: Invalid format of file name";
        return 400;
    }
    this->file.filename = filePart.substr(start, end - start);
    
    //content type
    key = "Content-Type: ";
    start = filePart.find(key);

    if(start == std::string::npos){
        errorMsg = "Multipart body: Missing file content type";
        return 400;
    }
    start += key.size();

    end = filePart.find(NEWLINE, start);
    if(end == std::string::npos){
        errorMsg = "Multipart body: Invalid format of file content type";
        return 400;
    }
    this->file.contentType = filePart.substr(start, end - start);

    //data
    start = filePart.find(HEADEND);

    if(start == std::string::npos){
        errorMsg = "Multipart body: Missing file data";
        return 400;
    }
    start += key.size();

    end = filePart.size();
    this->file.data = filePart.substr(start, end - start);
    
    return 0;
}

int HttpRequest::multipartParsing(){
    this->isMultipart = true;

    std::string contentType;
    tryGetHeader("content-type", contentType);

    size_t bpos = contentType.find("boundary=");
    this->boundary = contentType.substr(bpos + 9, contentType.size() - bpos);

    std::vector<std::string> multipart = split(body, boundary);
    if(multipart.empty()){
        errorMsg = "Multipart body: Invalid format";
        return 400;
    }
    return extractFirstFilePartToBody(multipart);
}

// to call after recv() function / true if the request information is complete
void HttpRequest::parseRequest(const std::string& recvBuffer, size_t bodyMaxSize){
    
    this->status = Parsing;
    this->statusCode = 0;
    std::string requestBuffer(recvBuffer);

    this->statusCode = readStartLine(requestBuffer);
    this->statusCode = readHeaders(requestBuffer);
    this->statusCode = readContentLengthBody(requestBuffer, bodyMaxSize);
    this->statusCode = readChunkedBody(requestBuffer, bodyMaxSize);
    
    if(detectMultipartAndBoundary()){
        this->statusCode = multipartParsing();
    }

    if(this->statusCode == 1)
        this->status = Incomplete;
    else if(this->statusCode == 0){
        this->status = Complete;
    }
    else{
        this->status = Error;
    }
}

void HttpRequest::printRequest(){
    std::cout << "\n-------REQUEST-BEGIN--------" << std::endl;
    std::cout << getMethod() << " " << getUri() << " " << getVersion() << std::endl;

    for(std::map<std::string, std::string>::iterator it = headers.begin(); it != headers.end(); it++){
        std::cout << it->first << ": " << it->second << std::endl;
    }

    if(!getBody().empty())
        std::cout << getBody() << std::endl;
    std::cout << "-------REQUEST-END--------\n" << std::endl;
}

bool HttpRequest::hasHeader(const std::string& key) const
{
	//lowercase keys during parsing
	return headers.find(key) != headers.end();
}

//to distinguish between non existant headers and empty ones
bool HttpRequest::tryGetHeader(const std::string& key, std::string& value) const
{
    std::map<std::string, std::string>::const_iterator it = headers.find(key);

    if (it == headers.end())
        return false;

    value = it->second;
    return true;
}

std::string HttpRequest::extensionFromContentType() const
{
    std::string contentType;
    tryGetHeader("content-type", contentType);
    return Mime::getExtension(contentType);
}

std::string HttpRequest::sanitizeFilename(const std::string& raw)
{
    std::string cleaned;
    cleaned.reserve(raw.size());

    for (size_t i = 0; i < raw.size(); ++i)
    {
        const char c = raw[i];
        const bool safe =
            (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') ||
            c == '.' || c == '_' || c == '-';
        if (safe)
            cleaned += c;
        else
            cleaned += '_';
    }

    while (!cleaned.empty() && cleaned[0] == '.')
        cleaned.erase(0, 1);

    if (cleaned.empty())
        return "";

    return cleaned;
}

std::string HttpRequest::filenameFromContentDisposition() const
{
    std::string header;
	tryGetHeader("content-disposition", header);

    if (header.empty())
        return "";

    size_t keyPos = header.find("filename=");
    if (keyPos == std::string::npos)
        return "";

    size_t valueStart = keyPos + 9;
    std::string rawName;
    if (valueStart < header.size() && header[valueStart] == '"')
    {
        ++valueStart;
        size_t valueEnd = header.find('"', valueStart);
        if (valueEnd == std::string::npos)
            return "";
        rawName = header.substr(valueStart, valueEnd - valueStart);
    }
    else
    {
        size_t valueEnd = header.find(';', valueStart);
        if (valueEnd == std::string::npos)
            valueEnd = header.size();
        rawName = header.substr(valueStart, valueEnd - valueStart);
    }

    if (rawName.empty())
        return "";

    // Keep basename only, discard any client-supplied path segments.
    size_t slashPos = rawName.find_last_of("/\\");
    if (slashPos != std::string::npos)
        rawName = rawName.substr(slashPos + 1);

    return sanitizeFilename(rawName);
}

bool HttpRequest::detectMultipartAndBoundary(){
    if((this->state != ReadingContentLengthBody && this->state != ReadingChunkedBody) || this->statusCode != 0)
        return false;
    
    std::string contentType;
    if(!tryGetHeader("content-type", contentType))
        return false;

    if(contentType.find("multipart/form-data;", 0) == std::string::npos)
        return false;
    
    if(contentType.find("boundary=", 0) == std::string::npos)
        return false;

    return true;
}
