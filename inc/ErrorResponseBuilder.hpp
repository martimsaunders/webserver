#pragma once
#include "HttpResponse.hpp"
#include "ServerConfig.hpp"

class ErrorResponseBuilder
{
	public:
		static HttpResponse build(int statusCode, const ServerConfig& server);
};
