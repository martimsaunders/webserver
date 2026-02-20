#pragma once
#include <iostream>

struct		Client
{
	int		fd;
	int		listen_fd;
	bool	want_write;
	bool	should_close;
	bool	header_parsed;
	size_t	server_index;
	size_t	expected_body;
	std::string in;
	std::string out;
};

struct		ListenSocket
{
	int		fd;
	int		port;
	size_t	server_index;
	std::string host;
};
