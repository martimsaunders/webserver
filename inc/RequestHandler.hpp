#pragma once
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "ServerConfig.hpp"

class RequestHandler
{
	public:
		static HttpResponse handleRequest(
			const HttpRequest& request,
			const ServerConfig& server
		);
};
