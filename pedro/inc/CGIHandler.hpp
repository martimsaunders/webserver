#pragma once
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Location.hpp"

class CGIHandler
{
public:
    HttpResponse execute(
        const HttpRequest& request,
        const Location& location,
        const std::string& scriptPath
    );
};
