#pragma once
#include "HttpResponse.hpp"
#include "ServerConfig.hpp"

//Factory Class (separation of concerns)
class ErrorResponseBuilder
{
	public:
		static HttpResponse build(int statusCode, const ServerConfig& server);
};
