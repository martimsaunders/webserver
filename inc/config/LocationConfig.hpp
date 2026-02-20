#pragma once
#include <string>
#include <vector>
#include <map>

struct LocationConfig
{
	int		redirect_code;
	bool	allow_get;
	bool	autoindex;
	bool	allow_post;
	bool	allow_delete;
	bool	has_redirect;
	bool	upload_enabled;
	size_t	client_max_body_size;
	std::string path;
	std::string root;
	std::string index;
	std::string upload_store;
	std::string redirect_target;
	std::map<int, std::string> error_pages;
	std::map<std::string, std::string> cgi;

	bool hasRedirect() const;
	bool isCgiRequest(const std::string& uri) const; //request
	bool matchesCgi(const std::string& extension) const; //maybe hardcoded
	std::string resolvePath(const std::string& uri) const; 
};

typedef LocationConfig Location;