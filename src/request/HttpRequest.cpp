#include "../../inc/request/HttpRequest.hpp"

HttpRequest::HttpRequest() : method(""), uri(""), version(""), headers(), body(""), errorMsg(""), 
statusCode(0), requestSize(0), status(Parsing), state(ReadingStartLine) {}
	
// Getters
std::string HttpRequest::getMethod() const { return method; }
std::string HttpRequest::getUri() const { return uri; }
std::string HttpRequest::getVersion() const { return version; }
std::string HttpRequest::getBody() const { return body; }
HttpRequest::ParseStatus HttpRequest::getStatus() const { return status; }
int HttpRequest::getStatusCode() const { return statusCode; }
int HttpRequest::getRequestSize() const { return requestSize; }

//Setters
void HttpRequest::setMethod(const std::string& m) { method = m; }
void HttpRequest::setUri(const std::string& u) { uri = u; }
void HttpRequest::setVersion(const std::string& v) { version = v; }
void HttpRequest::addHeader(const std::string& key, const std::string& value) {  headers[key] = value; }
void HttpRequest::setBody(const std::string& b) { body = b; }

std::vector<std::string> HttpRequest::split(std::string str, const std::string& c){
    std::vector<std::string> split;
    size_t pos = str.find(c);
    
    if(pos == std::string::npos){
        return split;
    }
    
    while(pos != std::string::npos){
        split.push_back(str.substr(0, pos));
        str.erase(0, pos + c.size());
        pos = str.find(c);
    }
    split.push_back(str);
    
    return split;
}

int HttpRequest::readStartLine(std::string& requestBuffer){
    if(this->state != ReadingStartLine || this->statusCode != 0)
        return this->statusCode;

    size_t pos = requestBuffer.find(NEWLINE);
    //incomplete request
    if(pos == std::string::npos)
        return 1;

    std::vector<std::string> startLine = split(requestBuffer.substr(0, pos), NEWLINE);
    requestBuffer.erase(0, pos + 2);
    this->requestSize += pos + 2;

    //invalid request line format
    if(startLine.size() != 3){
        errorMsg = "Start Line: Invalid request line format";
        return 400;
    }
    //invalid method
    if(startLine[0] != "GET" && startLine[0] != "POST" && startLine[0] != "DELETE" && startLine[0] != "HEAD" &&
        startLine[0] != "PUT  " && startLine[0] != "OPTIONS" && startLine[0] != "PATCH"){
        errorMsg = "Start Line: Invalid Method";
        return 405;
    }
    //unauthorized method
    if(startLine[0] != "GET" && startLine[0] != "POST" && startLine[0] != "DELETE"){
        this->setMethod("Unknown");
    }
    else
        this->setMethod(startLine[0]);

    //long URI
    if(startLine[1].size() > 8000){
        errorMsg = "Start Line: Long URI";
        return 414;
    }
    this->setUri(startLine[1]);

    //invalid version/protocol
    if(startLine[2] != "HTTP/1.0" && startLine[2] != "HTTP/1.1"){
        errorMsg = "Start Line: Invalid protocol/version";
        return 400;
    }
    this->setVersion(startLine[2]);

    return 0;
}

int HttpRequest::readHeaders(std::string& requestBuffer){
    if(this->state != ReadingHeaders || this->statusCode != 0)
        return this->statusCode;

    size_t pos = requestBuffer.find(HEADEND);
    //incomplete request
    if(pos == std::string::npos)
        return 1;

    if(requestBuffer.substr(0, pos).size() > 16000){
        errorMsg = "Headers: Headers field too large";
        return 400;
    }

    std::vector<std::string> headersLine = split(requestBuffer.substr(0, pos), NEWLINE);
    if(headersLine.empty()){
        errorMsg = "Headers: Bad format, no new lines";
        return 400;
    }
    requestBuffer.erase(0, pos + 4);
    this->requestSize += pos + 4;

    for(std::vector<std::string>::iterator it = headersLine.begin() + 1; it != headersLine.end(); it++){

        std::vector<std::string> header = split(*it, " ");
        //bad format
        if (header.size() != 2){
            errorMsg = "Header: Bad format";
            return 400;
        }
        //header delimiter check
        if(std::count(header[0].begin(), header[0].end(), ':') != 1){
            errorMsg = "Header: delimiter : error";
            return 400;
        }
        //invalid characters
        for(std::string::iterator i = header[0].begin(); i != header[0].end(); i++){
            if(!isalnum(*i) && *i != '-'){
                errorMsg = "Header: Invalid character";
                return 400;
            }
        }
        //line too long
        if((*it).size() > 8000){
            errorMsg = "Header: Header line too large";
            return 400;
        }
        
        this->addHeader(header[0], header[1]);
    }
    //Host header missing
    if(version == "HTTP/1.1" && !hasHeader("Host:")){
        errorMsg = "Header: Host header missing";
        return 400;
    }
    //two body types presence
    if(hasHeader("Transfer-Encoding:") && hasHeader("Content-Length:")){
        errorMsg = "Header: Two body types";
        return 400;
    }
    if(hasHeader("Content-Length:")){
        //non numeric Content-Length
        std::map<std::string, std::string>::iterator v = headers.find("Content-Length:");
        for(size_t i = 0; i < v->second.length(); i++){
            if(!isdigit(v->second[i])){
                errorMsg = "Header: Non numeris Content-Lenght value";
                return 400;
            }
        }
    }
    //duplicated headers
    for(std::map<std::string, std::string>::iterator it = headers.begin(); it != headers.end(); it++){
        if(it->first == "Host:" || it->first == "Content-Length:" || it->first == "Transfer-Encoding:" || it->first == "Content-Type:"){
            for(std::map<std::string, std::string>::iterator it2 = headers.begin(); it2 != headers.end(); it2++){
                if(it2->first == it->first){
                    errorMsg = "Header: Critical header duplicated";
                    return 400; 
                }
            }
        }
    }

    if(this->getMethod() == "POST"){

        if(hasHeader("Content-Length:"))
            this->state = ReadingContentLengthBody;
        else if(hasHeader("Transfer-Encoding:"))
            this->state = ReadingChunkedBody;
        else{
            errorMsg = "Header: Missing header";
            return 400;
        }
    }
    return 0;
}

int HttpRequest::readContentLengthBody(std::string& requestBuffer, size_t bodyMaxSize){
    if(this->state != ReadingContentLengthBody || this->statusCode != 0)
        return this->statusCode;
    
    std::map<std::string, std::string>::const_iterator it = headers.find("Content-Length:");
    size_t bodySize = static_cast<size_t>(atoi(it->second.c_str()));

    if(bodySize > bodyMaxSize){
        errorMsg = "Boddy: Body size too big";
        return 413;
    }
    //incomplete request
    if(requestBuffer.size() < bodySize)
        return 1;
    
    this->setBody(requestBuffer.substr(0, bodySize));
    this->requestSize += bodySize;
    return 0;
}

int HttpRequest::readChunkedBody(std::string& requestBuffer, size_t bodyMaxSize){
    if(this->state != ReadingChunkedBody || this->statusCode != 0)
        return this->statusCode;

    size_t pos = requestBuffer.find(HEADEND);
    //incomplete request
    if(pos == std::string::npos && requestBuffer.size() <= bodyMaxSize)
        return 1;
    
    if(pos > bodyMaxSize){
        errorMsg = "Boddy: Body size too big";
        return 413;
    }
    this->setBody(requestBuffer.substr(0, pos));
    this->requestSize += pos + 4;
    return 0;
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

    if(this->statusCode == 1)
        this->status = Incomplete;
    else if(this->statusCode == 0){
        // put header names in lowercase (map keys are immutable, so rebuild map)
        std::map<std::string, std::string> loweredHeaders;
        for(std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); it++){
            std::string key = it->first;
            std::transform(key.begin(), key.end(), key.begin(),
                static_cast<int (*)(int)>(std::tolower));
            loweredHeaders[key] = it->second;
        }
        headers.swap(loweredHeaders);
        this->status = Complete;
    }
    else
        this->status = Error;
}

void HttpRequest::printRequest(){
    std::cout << "-------REQUEST-BEGIN--------" << std::endl;
    std::cout << getMethod() << " " << getUri() << " " << getVersion() << std::endl;

    for(std::map<std::string, std::string>::iterator it = headers.begin(); it != headers.end(); it++){
        std::cout << it->first << " " << it->second << std::endl;
    }

    if(!getBody().empty())
        std::cout << getBody() << std::endl;
    std::cout << "-------REQUEST-END--------" << std::endl;
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
