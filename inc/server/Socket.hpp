#pragma once
#include <iostream>
#include "../request/HttpRequest.hpp"
#include "../request/RequestResult.hpp"

struct		Client
{
	int		fd;
	int		listen_fd;
	bool	want_write;
	bool	should_close;
	bool	header_parsed;
	bool	has_completed_request;
	size_t	server_index;
	size_t	expected_body;
	std::string in;
	std::string out;
	HttpRequest parser;
	unsigned long las_activity_tick;
	unsigned long las_write_progress_tick;
	RequestResult req_res;

};

struct		ListenSocket
{
	int		fd;
	int		port;
	size_t	server_index;
	std::string host;
};
