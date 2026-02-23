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

int HttpRequest::readStartLine(std::string& requestBuffer){
    if(this->state != ReadingStartLine || this->statusCode != 0)
        return this->statusCode;

    size_t pos = requestBuffer.find(NEWLINE);
    //incomplete request
    if(pos == std::string::npos)
        return 1;

    std::vector<std::string> startLine = split(requestBuffer.substr(0, pos), " ");
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

    this->state = ReadingHeaders;
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

    for(std::vector<std::string>::iterator it = headersLine.begin(); it != headersLine.end(); it++){

        std::vector<std::string> header = split(*it, ": ");
        //bad format
        if (header.size() != 2){
            errorMsg = "Header: Bad format";
            return 400;
        }
        //header delimiter check
        // if(std::count(header[0].begin(), header[0].end(), ':') != 1){
        //     errorMsg = "Header: delimiter : error";
        //     return 400;
        // }

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
        
        std::string key = header[0];
        std::transform(key.begin(), key.end(), key.begin(),
            static_cast<int (*)(int)>(std::tolower));
        this->addHeader(key, header[1]);
    }
    //Host header missing
    if(version == "HTTP/1.1" && !hasHeader("host")){
        errorMsg = "Header: Host header missing";
        return 400;
    }
    //two body types presence
    if(hasHeader("transfer-encoding") && hasHeader("content-length")){
        errorMsg = "Header: Two body types";
        return 400;
    }
    if(hasHeader("content-length")){
        //non numeric Content-Length
        std::map<std::string, std::string>::iterator v = headers.find("content-length");
        for(size_t i = 0; i < v->second.length(); i++){
            if(!isdigit(v->second[i])){
                errorMsg = "Header: Non numeris Content-Lenght value";
                return 400;
            }
        }
    }
    //duplicated headers
    for(std::map<std::string, std::string>::iterator it = headers.begin(); it != headers.end(); it++){
        if(it->first == "host" || it->first == "content-length" || it->first == "transfer-encoding" || it->first == "content-type"){
            for(std::map<std::string, std::string>::iterator it2 = headers.begin(); it2 != headers.end(); it2++){
                if(it2 == it)
                    continue;
                if(it2->first == it->first){
                    errorMsg = "Header: Critical header duplicated";
                    return 400; 
                }
            }
        }
    }

    if(this->getMethod() == "POST"){

        if(hasHeader("content-length"))
            this->state = ReadingContentLengthBody;
        else if(hasHeader("transfer-encoding"))
            this->state = ReadingChunkedBody;
        else{
            errorMsg = "Header: Missing body info header";
            return 400;
        }
    }
    return 0;
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
        errorMsg = "Body: Body size too big";
        return 413;
    }
    this->setBody(requestBuffer.substr(0, pos));
    this->requestSize += pos + 4;
    return 0;
}