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
	bool	is_cgi; //is cgi
	size_t	client_max_body_size;
	std::string path;
	std::string root;
	std::string index;
	std::string upload_store;
	std::string redirect_target;
	std::string interpreter_path; //cgi interpreter path
	std::map<int, std::string> error_pages;

	bool hasRedirect() const;
	std::string resolvePath(const std::string& uri) const; 
};

typedef LocationConfig Location;