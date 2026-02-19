#ifndef SERVER_CONFIG_HPP
# define SERVER_CONFIG_HPP

# include "LocationConfig.hpp"
# include <iostream>
# include <map>
# include <vector>

struct		ServerConfig
{
	int		port;
	size_t	client_max_body_size;
	std::string host;
	std::string root;
	std::string index;
	std::map<int, std::string> error_pages;
	std::vector<LocationConfig> locations;
};

#endif