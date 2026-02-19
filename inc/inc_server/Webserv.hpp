#ifndef WEBSERV_HPP
# define WEBSERV_HPP

# include "Socket.hpp"
# include "../inc_conf_pars/Config.hpp"
# include <map>
# include <poll.h>
# include <vector>
# include <string>

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
};

#endif