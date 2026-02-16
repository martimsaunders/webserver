#include "HttpRequest.hpp"

HttpRequest::HttpRequest() : method(""), uri(""), version(""), headers(), body(""), bodySize(0), status(Incomplete), state(ReadingStartLineAndHeaders) {}
	
// Getters
std::string HttpRequest::getMethod() const { return method; }
std::string HttpRequest::getUri() const { return uri; }
std::string HttpRequest::getVersion() const { return version; }
size_t HttpRequest::getBodySize() const { return bodySize; }

//Setters
void HttpRequest::setMethod(const std::string& m) { method = m; }
void HttpRequest::setUri(const std::string& u) { uri = u; }
void HttpRequest::setVersion(const std::string& v) { version = v; }
void HttpRequest::addHeader(const std::string& key, const std::string& value) {  headers[key] = value; }
void HttpRequest::setBody(const std::string& b) { body = b; }
void HttpRequest::setBodySize(size_t i) { bodySize = i; }

std::vector<std::string> HttpRequest::split(std::string str, const std::string& c){
    std::vector<std::string> split;
    size_t pos = str.find(c);
    
    if(pos == std::string::npos){
        throw std::runtime_error("Split [" + str + "] with [" + c + "] fail");
    }
    
    while(pos != std::string::npos){
        split.push_back(str.substr(0, pos));
        str.erase(0, pos + c.size());
        pos = str.find(c);
    }
    split.push_back(str);
    
    return split;
}

void HttpRequest::readStartLineAndHeaders(std::string& requestBuffer){
    size_t pos = requestBuffer.find(HEADEND);
    if(pos == std::string::npos)
        return;
        
    std::string head = requestBuffer.substr(0, pos);
    std::vector<std::string> headers = split(head, NEWLINE);

    std::vector<std::string> headerStart = split(headers.front(), " ");
    if(headerStart.size() != 3)
        throw std::runtime_error("Request start line format error: " + headers.front());

    this->setMethod(headerStart[0]);
    this->setUri(headerStart[1]);
    this->setVersion(headerStart[2]);

    for(std::vector<std::string>::iterator it = headers.begin() + 1; it != headers.end(); it++){
        std::vector<std::string> header = split(*it, " ");
        if (header.size() != 2)
            throw std::runtime_error("Request header lines format error: " + *it);
        
        this->addHeader(header[0], header[1]);
    }
    
    requestBuffer.erase(0, pos + 4);
    if(this->getMethod() == "POST"){
        std::string value;
        if(tryGetHeader("Transfer-Encoding:", value) && value == "chunked"){
            this->state = ReadingChunkedBody;
        }
        else if(hasHeader("Content-Length:")){
            this->state = ReadingContentLengthBody;
            this->setBodySize(static_cast<size_t>(atoi(this->headers.find("Content-Length:")->second.c_str())));
        }
    }
    else
        this->status = Complete;
}

void HttpRequest::readChunkedBody(std::string& requestBuffer){
    size_t pos = requestBuffer.find(HEADEND);
    if(pos == std::string::npos)
        return;
    this->setBody(requestBuffer.substr(0, pos));
    requestBuffer.erase(0, pos + 4);
    this->status = Complete;
}

// to call after recv() function / true if the request information is complete
bool HttpRequest::hasRequest(const std::string& recvBuffer){
    this->requestBuffer.append(recvBuffer);
    if (this->state == ReadingStartLineAndHeaders && this->status == Incomplete){
        readStartLineAndHeaders(requestBuffer);
    }
    if(this->state == ReadingContentLengthBody && this->status == Incomplete){
        if(requestBuffer.size() < this->getBodySize())
            return;
        this->setBody(requestBuffer.substr(0, this->getBodySize()));
        requestBuffer.erase(0, this->getBodySize());
        this->status = Complete;
    }
    else if(this->state == ReadingChunkedBody && this->status == Incomplete){
        readChunkedBody(requestBuffer);
    }
    if(this->status == Complete)
        return true;
    return false;
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

