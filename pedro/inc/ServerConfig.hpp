#pragma once
#include <string>
#include <vector>
#include <map>
#include "Location.hpp"

struct ServerConfig
{
	int		port;
	size_t	client_max_body_size;
	std::string host;
	std::string root;
	std::string index;
	std::map<int, std::string> error_pages;
	std::vector<LocationConfig> locations;

	// Helper
	const LocationConfig* findLocation(const std::string& uri) const;
};
