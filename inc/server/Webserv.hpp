#pragma once
#include "Socket.hpp"
#include "../config/Config.hpp"
#include <map>
#include <poll.h>
#include <vector>
#include <string>

class Webserv
{
	public:
		explicit Webserv(Config const &config);
		~Webserv();
		void run();

	private:
		Webserv(Webserv const &other);
		Webserv &operator=(Webserv const &other);

	private:
		Config _config;
		std::vector<pollfd> _pfds;
		std::map<int, ListenSocket> _listens;
		std::map<int, Client> _clients;

	private:
		int createListenFd(std::string const &host, int port);
		bool isListenFd(int fd) const;
		void closeAll();
		void addPollFd(int fd, short events);
		void delPollFd(int fd);
		void eventLoop();
		void setupListening();
		void acceptAll(int listen_fd);
		void handleClientRead(int fd);
		void handleClientWrite(int fd);
		void removeClient(int fd);
		void removeListen(int fd);
		void setPollEvents(int fd, short event);
	
	private:
		enum CgiFdRole { CGI_FD_STDIN, CGI_FD_STDOUT };

		struct CgiFdInfo{
			int client_fd;
			bool should_close;
			CgiFdRole role;
		};
		std::map<int, CgiFdInfo> _cgiFds;
		bool isCgiFd(int fd) const;
		void handleCgiRead(int cgi_fd);
		void handleCgiWrite(int cgi_fd);
		void startDeferredCgiForClient(int client_fd);
};
