#pragma once
#include <string>
#include <fstream>
#include "Socket.hpp"

#define LOGGER false

class Logger
{
	public:
		static bool init(std::string const &path);
		static void shutdown();

		static void accept(int client_fd, int listen_fd);
		static void response(int fd, ResponseLog const &log, int listen_fd);
		static void closeClient(int fd, std::string const &reason, int listen_fd);
		static void closeListen(std::string const &reason, int listen_fd);
		static void listen(std::string const &host, int port, size_t server_index);
		static void shutdownMsg(std::string const &msg);

	private:
		Logger();
		static std::ofstream _file;

		static std::string timestamp();
		static void writeFileLine(std::string const &line);
};