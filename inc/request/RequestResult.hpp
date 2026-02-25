#pragma once
#include "../response/HttpResponse.hpp"
#include <string>
#include <sys/types.h>

struct CgiStartData
{
    bool active;
    pid_t pid;
    int stdinFd;
    int stdoutFd;
    std::string requestBody;

    CgiStartData() : active(false), pid(-1), stdinFd(-1), stdoutFd(-1), requestBody("") {}
};

struct RequestResult
{
    bool deferred;
    HttpResponse response;
    CgiStartData cgi;
    std::string cgiRawOutput;

    RequestResult() : deferred(false), response(), cgi(), cgiRawOutput("") {}

    static RequestResult immediate(const HttpResponse& resp)
    {
        RequestResult result;
        result.deferred = false;
        result.response = resp;
        return result;
    }

    static RequestResult deferredCgi(const CgiStartData& start)
    {
        RequestResult result;
        result.deferred = true;
        result.cgi = start;
        result.cgi.active = true;
        return result;
    }
};
