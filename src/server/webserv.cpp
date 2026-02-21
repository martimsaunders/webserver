#include "../../inc/server/Webserv.hpp"
#include "../../inc/request/RequestHandler.hpp"
#include "../../inc/response/HttpResponse.hpp"
#include "../../inc/response/ResponseBuilder.hpp"
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
#include <set>
#include <csignal>
#include <ctime>

#define WRITE_STALL_TICKS 10
#define CONNECT_IDLE_TICKS 15
#define HEADER_TIMEOUT_TICKS 10
#define KEEPALIVE_TICKS 30

static volatile sig_atomic_t g_stop = 0;
static unsigned long g_tick = 0;

static void handleStopSignal(int){
	g_stop = 1;
}

//constructors e destructor
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

//helper functions
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



//member functions
void Webserv::run(){
	std::signal(SIGINT, handleStopSignal);
	std::signal(SIGTERM, handleStopSignal);
	std::signal(SIGQUIT, handleStopSignal);
	std::signal(SIGPIPE, SIG_IGN);
	std::signal(SIGTSTP, SIG_IGN);
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
		if (g_stop){
			closeAll();
			std::cout << "Shutting down server gracefully..." << std::endl;
			return;
		}
		if (_pfds.empty())
			throw std::runtime_error("no fds to poll()");
		int rc = poll(&_pfds[0], _pfds.size(), 1000);
		++g_tick;
		if (rc < 0){
			if (errno == EINTR)
				continue;
			throw std::runtime_error("poll() failed");
		}
		std::set<int> toRemove;
		std::set<int> toRemoveListens;
		for (std::map<int, Client>::iterator cit = _clients.begin(); cit != _clients.end(); cit++){
			Client &c = cit->second;
			if (!c.out.empty() && (g_tick - c.las_write_progress_tick) > WRITE_STALL_TICKS){
				toRemove.insert(cit->first);
				continue;
			}
			if (!c.in.empty() && c.out.empty()){
				if ((g_tick - c.las_activity_tick) > HEADER_TIMEOUT_TICKS)
					toRemove.insert(cit->first);
				continue;
			}
			if (c.in.empty() && c.out.empty()){
				if (!c.has_completed_request){
					if ((g_tick - c.las_activity_tick) > CONNECT_IDLE_TICKS)
						toRemove.insert(cit->first);
				}
				else{
					if ((g_tick - c.las_activity_tick) > KEEPALIVE_TICKS)
						toRemove.insert(cit->first);
				}
			}
		}
		size_t n = _pfds.size();
		for (size_t i = 0; i < n; i++){
			int fd = _pfds[i].fd;
			short re = _pfds[i].revents;
			if (re == 0)
				continue;
			if (isListenFd(fd)){
				if (re & (POLLHUP | POLLERR | POLLNVAL)){
					toRemoveListens.insert(fd);
					continue;
				}
				if (re & POLLIN)
					acceptAll(fd);
			}
			else{
				if (re & (POLLHUP | POLLERR | POLLNVAL)){
					toRemove.insert(fd);
					continue;
				}
				if (re & POLLIN)
					handleClientRead(fd);
				std::map<int , Client>::iterator it = _clients.find(fd);
				if (it != _clients.end() && it->second.should_close && it->second.out.empty()){
					toRemove.insert(fd);
					continue;
				}
				if (re & POLLOUT)
					handleClientWrite(fd);
				it = _clients.find(fd);
				if (it != _clients.end() && it->second.should_close && it->second.out.empty())
					toRemove.insert(fd);
			}
		}
		for (std::set<int>::iterator it = toRemove.begin(); it != toRemove.end(); it++)
			removeClient(*it);
		for (std::set<int>::iterator it = toRemoveListens.begin(); it != toRemoveListens.end(); it++){
			removeListen(*it);
			std::cout << "Closed listen_fd=" << *it << std::endl;
			if (_listens.empty()){
				closeAll();
				throw std::runtime_error("all listen sockets failed");
			}
		}
	}
}

void Webserv::acceptAll(int listen_fd){
	while (true){
		sockaddr_in client;
		std::memset(&client, 0, sizeof(client));
		socklen_t client_len = sizeof(client);
		int client_fd = accept(listen_fd, reinterpret_cast<sockaddr*>(&client), &client_len);
		if (client_fd < 0){
			if (errno == EINTR)
				continue;
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				return;
			std::cerr << "accept() failed on listen_fd=" << listen_fd << std::endl;
			return;
		}
		if (setNonBlocking(client_fd) < 0){
			close(client_fd);
			continue;
		}
		std::map<int, ListenSocket>::iterator it =_listens.find(listen_fd);
		if (it == _listens.end()){
			close(client_fd);
			return;
		}
		Client c;
		c.fd = client_fd;
		c.listen_fd = listen_fd;
		c.want_write = false;
		c.should_close = false;
		c.header_parsed = false;
		c.has_completed_request = false;
		c.las_activity_tick = g_tick;
		c.las_write_progress_tick = g_tick;
		c.server_index = it->second.server_index;
		c.expected_body = _config.servers[c.server_index].client_max_body_size;
		_clients[client_fd] = c;
		addPollFd(client_fd, POLLIN); 
		std::cout << "Accepted client fd=" << client_fd << " on listen_fd=" << listen_fd << std::endl;
	}
}

void Webserv::setPollEvents(int fd, short event){
	for (size_t i = 0; i < _pfds.size(); i++){
		if (_pfds[i].fd == fd){
			_pfds[i].events = event;
			return ;
		}
	}
}

void Webserv::handleClientRead(int fd){
	std::map<int, Client>::iterator it = _clients.find(fd);
	if (it == _clients.end())
		return;
	Client &client = it->second;
	if (!client.out.empty())
		return ;
	char temp[4096];
	ssize_t len = recv(fd, temp, sizeof(temp), 0);
	if (len > 0){
		client.in.append(temp, len);
		client.las_activity_tick = g_tick;
	}
	else if (len == 0){
		client.should_close = true;
		return ;
	}
	else{
		return;
	}
	client.parser.parseRequest(client.in, client.expected_body);
	if (client.parser.getStatus() == HttpRequest::Incomplete)
		return;
	else if (client.parser.getStatus() == HttpRequest::Complete){
		HttpResponse resp = RequestHandler::handleRequest(client.parser, _config.servers[client.server_index]);
		client.in.erase(0, client.parser.getRequestSize());
		client.out = resp.toString();
		client.las_write_progress_tick = g_tick;
		client.has_completed_request = true;
		setPollEvents(fd, POLLOUT);
		client.parser = HttpRequest();
		return ;
	}
	HttpResponse err = ResponseBuilder::buildErrorResponse(client.parser.getStatusCode(), _config.servers[client.server_index]);
	client.out = err.toString();
	client.should_close = true;
	client.las_write_progress_tick = g_tick;
	client.has_completed_request = true;
	setPollEvents(fd, POLLOUT);
	client.parser = HttpRequest();
}

void Webserv::handleClientWrite(int fd){
	std::map<int, Client>::iterator it = _clients.find(fd);
	if (it == _clients.end())
		return;
	Client &client = it->second;
	if (client.out.empty()){
		setPollEvents(fd, POLLIN);
		return ;
	}
	ssize_t sent = send(fd, client.out.data(), client.out.size(), 0);
	if (sent > 0){
		client.out.erase(0, sent);
		client.las_write_progress_tick = g_tick;
		client.las_activity_tick = g_tick;
	}
	else {
		setPollEvents(fd, POLLOUT);
		return ;
	}
	if (!client.out.empty()){
		setPollEvents(fd, POLLOUT);
		return ;
	}
	if (client.should_close)
		return;
	setPollEvents(fd, POLLIN);
	if (!client.in.empty()){
		client.parser.parseRequest(client.in, client.expected_body);
		if (client.parser.getStatus() == HttpRequest::Incomplete)
			return;
		else if (client.parser.getStatus() == HttpRequest::Complete){
			HttpResponse resp = RequestHandler::handleRequest(client.parser, _config.servers[client.server_index]);
			client.in.erase(0, client.parser.getRequestSize());
			client.out = resp.toString();
			client.las_write_progress_tick = g_tick;
			setPollEvents(fd, POLLOUT);
			client.parser = HttpRequest();
			return ;
		}
		HttpResponse err = ResponseBuilder::buildErrorResponse(client.parser.getStatusCode(), _config.servers[client.server_index]);
		client.out = err.toString();
		client.should_close = true;
		client.las_write_progress_tick = g_tick;
		setPollEvents(fd, POLLOUT);
		client.parser = HttpRequest();
	}
}

void Webserv::removeClient(int fd){
	int listen_fd = -1;
	std::map<int, Client>::iterator it = _clients.find(fd);
	if (it != _clients.end())
		listen_fd = it->second.listen_fd;
	delPollFd(fd);
	close(fd);
	if (it != _clients.end())
		_clients.erase(it);
	std::cout << "Close client fd=" << fd;
	if (listen_fd != -1)
		std::cout << " on listen_fd=" << listen_fd;
	std::cout << std::endl;
}

void Webserv::removeListen(int fd){
	delPollFd(fd);
	close(fd);
	_listens.erase(fd);
}
