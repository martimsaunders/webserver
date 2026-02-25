#include "../../inc/request/HttpRequest.hpp"

static bool isUriChar(char c){
    if(isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~'
     || c == '!' || c == '$' || c == '&' || c == '\'' || c == '(' 
     || c == ')' || c == '*' || c == '+' || c == ',' || c == ';' 
     || c == '=' || c == ':' || c == '@' || c == '%' || c == '/')
        return true;
    return false;
}

bool HttpRequest::validateMethod(const std::vector<std::string>& startLine){
    //invalid method
    if(startLine[0] != "GET" && startLine[0] != "POST" && startLine[0] != "DELETE" && startLine[0] != "HEAD" &&
        startLine[0] != "PUT  " && startLine[0] != "OPTIONS" && startLine[0] != "PATCH"){
        errorMsg = "Method: Invalid Method";
        statusCode = 405;
        return false;
    }
    //unauthorized method
    if(startLine[0] != "GET" && startLine[0] != "POST" && startLine[0] != "DELETE"){
        this->setMethod("Unknown");
    }
    else
        this->setMethod(startLine[0]);
    return true;
}

static bool isHex(char c) {
    return (c >= '0' && c <= '9')
        || (c >= 'a' && c <= 'f')
        || (c >= 'A' && c <= 'F');
}

static unsigned char hexVal(char c) {
    if (c >= '0' && c <= '9') return static_cast<unsigned char>(c - '0');
    if (c >= 'a' && c <= 'f') return static_cast<unsigned char>(10 + (c - 'a'));
    return static_cast<unsigned char>(10 + (c - 'A')); // 'A'..'F'
}

// Return true if decode OK; false if detect decode error
// out gets the decoded string
static bool percentDecode(const std::string& in, std::string& out, std::string& errorMsg) {
    out.clear();
    out.reserve(in.size());

    for (std::size_t i = 0; i < in.size(); ) {
        char c = in[i];

        if (c != '%') {
            // reject raw control characters
            unsigned char uc = static_cast<unsigned char>(c);
            if (uc < 0x20 || uc == 0x7F){
                errorMsg = "URI: raw control character";
                return false;
            }

            out.push_back(c);
            ++i;
            continue;
        }

        if (i + 2 >= in.size()) {
            // incomplete: "%", "%A" etc.
            errorMsg = "URI %: Incomplete percent encoded char";
            return false;
        }

        char h1 = in[i + 1];
        char h2 = in[i + 2];
        if (!isHex(h1) || !isHex(h2)) {
            errorMsg = "URI %: non hex character";
            return false;
        }

        unsigned char v = static_cast<unsigned char>((hexVal(h1) << 4) | hexVal(h2));

        // reject NULL and controls (CR/LF)
        if (v == 0x00 || v == 0x0D || v == 0x0A || v < 0x20 || v == 0x7F){
            errorMsg = "URI %: null and control char detect";
            return false;
        }

        out.push_back(static_cast<char>(v));
        i += 3;
    }

    return true;
}

bool HttpRequest::validateURI(const std::vector<std::string>& startLine){
    //long URI
    if(startLine[1].size() > 8000){
        errorMsg = "Uri: Long URI";
        statusCode = 414;
        return false;
    }
    this->setUri(startLine[1]);
    
    size_t start = uri.find("?");
    if(start != std::string::npos){
        // if(std::count(uri.begin(), uri.end(), '?') > 1){
        //     errorMsg = "Uri: Multiple '?'";
        //     statusCode = 400;
        //     return false;
        // }
        
        //uri query invalid char
        queryString = uri.substr(start + 1, uri.size() - start);
        for(size_t i = 0; i < queryString.size(); i++){
            if(!isUriChar(queryString[i])){
                errorMsg = "Uri: Ivalid char";
                statusCode = 400;
                return false;
            }
        }
        uriPath = uri.substr(0, start);
    }
    else
        uriPath = uri;
    //uri invalid char
    for(size_t i = 0; i < uriPath.size(); i++){
        if(!isUriChar(uriPath[i])){
            errorMsg = "Uri: Ivalid char";
            statusCode = 400;
            return false;
        }
    }

    //percent-encoded uri
    std::string tempUri;
    if(uriPath.find('%') != std::string::npos){
        if(!percentDecode(uriPath, tempUri, errorMsg)){
            statusCode = 400;
            return false;
        }
    }
    else
        tempUri = uriPath;

    //uri don't start with /
    if(tempUri[0] != '/'){
        errorMsg = "Uri: Don't start with '/'";
        statusCode = 400;
        return false;
    }
    
    for(std::string::const_iterator cIt = tempUri.begin(); cIt != tempUri.end(); cIt++){
        if(*cIt == '/' && (cIt + 1) != tempUri.end() && *(cIt + 1) == '/'){
            errorMsg = "Uri: // detected";
            statusCode = 400;
            return false;
        }
    }

    // /.. detected
    std::vector<std::string> paths = split(tempUri, "/");
    for(std::vector<std::string>::const_iterator cIt = paths.begin(); cIt != paths.end(); cIt++){
        if(*cIt == ".."){
            errorMsg = "Uri: /../ detected";
            statusCode = 400;
            return false;
        }
    }
    return true;
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
    
    if(!validateMethod(startLine))
        return statusCode;

    if(!validateURI(startLine))
        return statusCode;

    //invalid version/protocol
    if(startLine[2] != "HTTP/1.0" && startLine[2] != "HTTP/1.1"){
        errorMsg = "Protocol/Version: Invalid protocol/version";
        return 400;
    }
    this->setVersion(startLine[2]);

    this->state = ReadingHeaders;
    return 0;
}