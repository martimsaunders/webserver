#include "../../inc/request/HttpRequest.hpp"

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