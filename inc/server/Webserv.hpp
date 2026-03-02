#pragma once
#include "Socket.hpp"
#include "../config/Config.hpp"
#include <map>
#include <poll.h>
#include <vector>
#include <string>

#define GREEN "\033[0;32m"
#define BLUE "\033[0;34m"
#define RED "\033[0;31m"
#define YELLOW "\033[0;33m"
#define MAGENTA "\033[0;35m"
#define CYAN "\033[0;36m"
#define PURPLE "\033[0;35m"
#define WHITE "\033[0;37m"
#define RESET "\033[0m"
#define BOLD_GREEN "\033[1;32m"
#define BOLD_BLUE "\033[1;34m"
#define BOLD_RED "\033[1;31m"
#define BOLD_YELLOW "\033[1;33m"
#define BOLD_MAGENTA "\033[1;35m"
#define BOLD_CYAN "\033[1;36m"
#define BOLD_WHITE "\033[1;97m"

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
		void removeClient(int fd, std::string const &reason);
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
