#include "../../inc/request/HttpRequest.hpp"

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

int HttpRequest::readContentLengthBody(std::string& requestBuffer, size_t bodyMaxSize){
    if(this->state != ReadingContentLengthBody || this->statusCode != 0)
        return this->statusCode;
    
    std::map<std::string, std::string>::const_iterator it = headers.find("content-length");
    size_t bodySize = static_cast<size_t>(atoi(it->second.c_str()));

    if(bodySize > bodyMaxSize){
        errorMsg = "Body: Body size too big";
        return 413;
    }
    //incomplete request
    if(requestBuffer.size() < bodySize)
        return 1;
    
    this->body = requestBuffer.substr(0, bodySize);
    requestBuffer.erase(0, bodySize);

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
        errorMsg = "Body: Body size too big";
        return 413;
    }
    this->setBody(requestBuffer.substr(0, pos));
    requestBuffer.erase(0, pos + 4);

    this->requestSize += pos + 4;
    return 0;
}