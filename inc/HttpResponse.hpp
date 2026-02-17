#pragma once
#include <string>
#include <map>

class HttpResponse
{
	private:
		int statusCode;
		std::map<std::string, std::string> headers;
		std::string body;

	public:
		HttpResponse();

		// Helper methods
		static HttpResponse buildRedirect(const std::string& url);
};
