/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mprazere <mprazere@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/16 16:24:48 by mprazere          #+#    #+#             */
/*   Updated: 2026/02/16 16:35:59 by mprazere         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
# define SERVER_HPP

# include "Socket.hpp"
# include "../inc_conf_pars/ServerConfig.hpp"
# include <map>
# include <poll.h>
# include <vector>

class Server
{
	public:
		Server(ServerConfig const &config);
		void run();

	private:
		ServerConfig const &_config;
		std::vector<pollfd> _pfds;
		std::map<int, ListenSocket> _listens;
		std::map<int, Client> _clients;

	private:
		int createListenFd(std::string const &host, int port);
		void acceptAll(int listen_fd);
		void handleClientRead(int fd);
		void handleClientWrite(int fd);
		void removeClient(int fd);
};

#endif