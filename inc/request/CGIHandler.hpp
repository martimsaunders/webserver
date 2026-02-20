#pragma once
#include "HttpRequest.hpp"
#include "../response/HttpResponse.hpp"
#include "../config/LocationConfig.hpp"

class CGIHandler
{
public:
    HttpResponse execute(
        const HttpRequest& request,
        const Location& location,
        const std::string& scriptPath
    );
};
