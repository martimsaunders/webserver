/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webserv.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: praders <praders@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/17 12:20:57 by praders           #+#    #+#             */
/*   Updated: 2026/02/17 17:12:03 by praders          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../inc/inc_server/Webserv.hpp"
#include <iostream>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sstream>

Webserv::Webserv(Config const &config): _config(config){}

Webserv::Webserv(Webserv const &other): _config(other._config), _pfds(other._pfds), _listens(other._listens), _clients(other._clients){}

Webserv &Webserv::operator=(Webserv const &other){
	if (this != &other){
		_config = other._config;
		_pfds = other._pfds;
		_listens = other._listens;
		_clients = other._clients;
	}
	return (*this);
}

Webserv::~Webserv(){
	closeAll();
}

void Webserv::run(){
	setupListening();
	eventLoop();
}

void Webserv::addPollFd(int fd, short events){
	pollfd p;
	p.fd = fd;
	p.events = events;
	p.revents = 0;
	_pfds.push_back(p);
}

void Webserv::delPollFd(int fd){
	for (size_t i = 0; i < _pfds.size(); i++){
		if (_pfds[i].fd == fd){
			_pfds.erase(_pfds.begin() + i);
			return;
		}
	}
}

void Webserv::setupListening(){
	for (size_t i = 0; i < _config.servers.size(); i++ ){
		ServerConfig const &sc = _config.servers[i];
		int fd = createListenFd(sc.host, sc.port);
		ListenSocket ls;
		ls.fd = fd;
		ls.port = sc.port;
		ls.server_index = i;
		ls.host = sc.host;
		_listens[fd] = ls;
		addPollFd(fd, POLLIN);
		std::cout << "Listening on ";
		if (sc.host.empty()) std::cout << "0.0.0.0";
		else std::cout << sc.host;
		std::cout << ":" << sc.port << " (server_index=" << i << ")" << std::endl;
	}
}

bool Webserv::isListenFd(int fd) const{
	return (_listens.find(fd) != _listens.end());
}

static int setNonBlocking(int fd){
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1) return (-1);
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) return (-1);
	return (0);
}

static std::string toString(int port){
	std::ostringstream oss;
	oss << port;
	return (oss.str());
}

int Webserv::createListenFd(std::string const &host, int port){
	std::string portStr = toString(port);
	struct addrinfo hints;
	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	char const *node = NULL;
	if (!host.empty() && host != "0.0.0.0")
		node = host.c_str();
	if (node == NULL)
		hints.ai_flags = AI_PASSIVE;
	else
		hints.ai_flags = 0;
	struct addrinfo *res = NULL;
	int rc = getaddrinfo(node, portStr.c_str(), &hints, &res);
	if (rc != 0 || res == NULL){
		if (res)
			freeaddrinfo(res);
		throw std::runtime_error("invalid listen host (getaddrinfo failed)");
	}
	int fd = -1;
	for (struct addrinfo *p = res; p != NULL; p = p->ai_next){
		fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (fd < 0)
			continue;
		int yes = 1;
		if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0){
			close(fd);
			fd = -1;
			continue;
		}
		if (setNonBlocking(fd) < 0){
			close(fd);
			fd = -1;
			continue;
		}
		if (bind(fd, p->ai_addr, p->ai_addrlen) < 0){
			close(fd);
			fd = -1;
			continue;
		}
		if (listen(fd, 128) < 0){
			close(fd);
			fd = -1;
			continue;
		}
		break;
	}
	freeaddrinfo(res);
	if (fd < 0)
		throw std::runtime_error("failed to create listen socket");
	return (fd);
}

void Webserv::closeAll(){
	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); it++)
		close(it->first);
	_clients.clear();
	for (std::map<int, ListenSocket>::iterator it = _listens.begin(); it != _listens.end(); it++)
		close(it->first);
	_listens.clear();
	_pfds.clear();
}

void Webserv::eventLoop(){
	while (true){
		if (_pfds.empty())
			throw std::runtime_error("no fds to poll()");
		int rc = poll(&_pfds[0], _pfds.size(), 1000);
		if (rc < 0){
			if (errno == EINTR)
				continue;
			throw std::runtime_error("poll() failed");
		}
		std::vector<int> toRemove;
		size_t n = _pfds.size();
		for (size_t i = 0; i < n; i++){
			int fd = _pfds[i].fd;
			short re = _pfds[i].revents;
			if (re == 0)
				continue;
			if (isListenFd(fd)){
				if (re & POLLIN)
					acceptAll(fd);
			}
			else{
				if (re & POLLIN)
					handleClientRead(fd);
				std::map<int , Client>::iterator it = _clients.find(fd);
				if (it != _clients.end() && it->second.should_close){
					toRemove.push_back(fd);
					continue;
				}
				if (re & POLLOUT)
					handleClientWrite(fd);
				it = _clients.find(fd);
				if (it != _clients.end() && it->second.should_close)
					toRemove.push_back(fd);
			}
		}
		for (size_t j = 0; j < toRemove.size(); j++)
			removeClient(toRemove[j]);
	}
}

void Webserv::acceptAll(int listen_fd){
	while (true){
		sockaddr_in client;
		std::memset(&client, 0, sizeof(client));
		socklen_t client_len = sizeof(client);
		int client_fd = accept(listen_fd, reinterpret_cast<sockaddr*>(&client), &client_len);
		if (client_fd < 0){
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				return;
			throw std::runtime_error("accept() failed");
		}
		if (setNonBlocking(client_fd) < 0)
			throw std::runtime_error("fcntl(O_NONBLOCK) failed");
		Client c;
		c.fd = client_fd;
		c.listen_fd = listen_fd;
		c.want_write = false;
		c.should_close = false;
		c.header_parsed = false;
		c.expected_body = 0;
		std::map<int, ListenSocket>::iterator it =_listens.find(listen_fd);
		if (it == _listens.end()){
			close(client_fd);
			throw std::runtime_error("internal error: accept on unknown listen_fd");
		}
		c.server_index = it->second.server_index;
		_clients[client_fd] = c;
		addPollFd(client_fd, POLLIN); 
		std::cout << "Accepted client fd=" << client_fd << " on listen_fd=" << listen_fd << std::endl;
	}
}

void Webserv::handleClientRead(int fd){
	(void) fd;
}

void Webserv::handleClientWrite(int fd){
	(void) fd;
}

void Webserv::removeClient(int fd){
	delPollFd(fd);
	close(fd);
	_clients.erase(fd);
}