#include "../../inc/server/Webserv.hpp"
#include "../../inc/request/CGIHandler.hpp"
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
#include <sys/wait.h>
#include <signal.h>

#define WRITE_STALL_TICKS 10
#define CONNECT_IDLE_TICKS 15
#define HEADER_TIMEOUT_TICKS 10
#define KEEPALIVE_TICKS 30
#define CGI_TIMEOUT_TICKS 120

//========================================================================
// Global loop state (signals + logical "tick")
//========================================================================
// Flag set by signal handlers to request a graceful shutdown.
static volatile sig_atomic_t g_stop = 0;

// Loop "tick" (increments every iteration). Used for simple timeouts.
static unsigned long g_tick = 0;

// Signal handler: marks shutdown request (event loop checks and exits).
static void handleStopSignal(int){
	g_stop = 1;
}

//========================================================================
// Webserv: constructors / destructor
//========================================================================
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

// Close all sockets and clear internal structures.
Webserv::~Webserv(){
	closeAll();
}

//========================================================================
// Socket helpers
//========================================================================
// Put a file descriptor in non-blocking mode (required for poll + accept/recv/send).
static int setNonBlocking(int fd){
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1) return (-1);
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) return (-1);
	return (0);
}

// Convert port to string (for getaddrinfo).
static std::string toString(int port){
	std::ostringstream oss;
	oss << port;
	return (oss.str());
}

static void safeCloseFd(int &fd){
	if (fd >= 0){
		close(fd);
		fd = -1;
	}
}

//========================================================================
// Webserv: member functions
//========================================================================
// Server entry point:
// - installs signal handlers
// - creates listening sockets
// - enters the poll-based event loop
void Webserv::run(){
	std::signal(SIGINT, handleStopSignal);
	std::signal(SIGTERM, handleStopSignal);
	std::signal(SIGQUIT, handleStopSignal);

	// Avoid crash when calling send() on a socket closed by the peer.
	std::signal(SIGPIPE, SIG_IGN);

	// Ignore Ctrl+Z (optional)
	std::signal(SIGTSTP, SIG_IGN);

	setupListening();
	eventLoop();
}

// Add fd to the poll array.
void Webserv::addPollFd(int fd, short events){
	pollfd p;
	p.fd = fd;
	p.events = events;
	p.revents = 0;
	_pfds.push_back(p);
}


// Remove fd from the poll array.
void Webserv::delPollFd(int fd){
	for (size_t i = 0; i < _pfds.size(); i++){
		if (_pfds[i].fd == fd){
			_pfds.erase(_pfds.begin() + i);
			return;
		}
	}
}

// Create one listening socket per server in the config and add it to poll (POLLIN).
void Webserv::setupListening(){
	for (size_t i = 0; i < _config.servers.size(); i++ ){
		ServerConfig const &sc = _config.servers[i];

		int fd = createListenFd(sc.host, sc.port);

		// Store listen socket metadata (host/port/server_index).
		ListenSocket ls;
		ls.fd = fd;
		ls.port = sc.port;
		ls.server_index = i;
		ls.host = sc.host;
		_listens[fd] = ls;

		addPollFd(fd, POLLIN);
		std::cout << "Listening on ";

		// Simple log
		if (sc.host.empty()) std::cout << "0.0.0.0";
		else std::cout << sc.host;
		std::cout << ":" << sc.port << " (server_index=" << i << ")" << std::endl;
	}
}

// Check if fd belongs to the listen sockets map.
bool Webserv::isListenFd(int fd) const{
	return (_listens.find(fd) != _listens.end());
}

// Create, configure and bind/listen a TCP socket (IPv4) for host:port.
// Uses getaddrinfo and tries all returned addresses until it succeeds.
int Webserv::createListenFd(std::string const &host, int port){
	std::string portStr = toString(port);

	struct addrinfo hints;
	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	// If host is empty or 0.0.0.0, use AI_PASSIVE to bind on all interfaces.
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

	// Try socket/bind/listen for each addrinfo result.
	for (struct addrinfo *p = res; p != NULL; p = p->ai_next){
		fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (fd < 0)
			continue;

		// Fast port reuse after restart.
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

		// Sucess
		break;
	}

	freeaddrinfo(res);

	if (fd < 0)
		throw std::runtime_error("failed to create listen socket");

	return (fd);
}

// Close all sockets (clients + listens) and clear internal structures.
void Webserv::closeAll(){
	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); it++)
		close(it->first);
	_clients.clear();

	for (std::map<int, ListenSocket>::iterator it = _listens.begin(); it != _listens.end(); it++)
		close(it->first);
	_listens.clear();

	_pfds.clear();
}

//========================================================================
// eventLoop
//========================================================================
// Main poll-based event loop:
// - poll() with fixed timeout
// - accept new clients on listen fds
// - read requests (POLLIN) and write responses (POLLOUT)
// - apply timeouts (header/keep-alive/write stall)
// - remove invalid clients/listeners
void Webserv::eventLoop(){
	while (true){
		// Graceful shutdown via signals.
		if (g_stop){
			closeAll();
			std::cout << "Shutting down server gracefully..." << std::endl;
			return;
		}
		if (_pfds.empty())
			throw std::runtime_error("no fds to poll()");

		// poll timeout 1000ms: wake up to update timeouts and react to g_stop
		int rc = poll(&_pfds[0], _pfds.size(), 1000);
		++g_tick;

		if (rc < 0){
			// poll interrupted by signal -> restart loop
			if (errno == EINTR)
				continue;
			throw std::runtime_error("poll() failed");
		}

		// Sets of fds to remove (remove outside loops to avoid invalidating iterators).
		std::set<int> toRemove;
		std::set<int> toRemoveCgi;
		std::set<int> toRemoveListens;

		//-----------------------------------------------------------------
		// Per-client timeouts (tick-based)
		//-----------------------------------------------------------------
		for (std::map<int, Client>::iterator cit = _clients.begin(); cit != _clients.end(); cit++){
			Client &c = cit->second;

			// While CGI is running, uses its own timeout
			if (c.req_res.deferred && c.req_res.cgi.active){
				if ((g_tick - c.las_activity_tick) > CGI_TIMEOUT_TICKS)
					toRemove.insert(cit->first);
				continue;
			}

			// If we have been writing for too long without progress -> remove
			if (!c.out.empty() && (g_tick - c.las_write_progress_tick) > WRITE_STALL_TICKS){
				toRemove.insert(cit->first);
				continue;
			}

			// If we received something (in not empty) but have no output yet,
			// treat as "waiting for full header/request".
			if (!c.in.empty() && c.out.empty()){
				if ((g_tick - c.las_activity_tick) > HEADER_TIMEOUT_TICKS)
					toRemove.insert(cit->first);
				continue;
			}

			// If both buffers are empty, distinguish:
			// - no request completed yet -> idle connect timeout
			// - at least one request completed -> keep-alive timeout
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

		//-----------------------------------------------------------------
		// Process poll events
		//-----------------------------------------------------------------
		size_t n = _pfds.size();
		for (size_t i = 0; i < n; i++){
			int fd = _pfds[i].fd;
			short re = _pfds[i].revents;

			if (re == 0)
				continue;
			
			//-----------------------------------------------------------------
			// CGI fds
			//-----------------------------------------------------------------
			if (isCgiFd(fd)){
				// If there is an error/hup/navl, mark this fd (and the ones from the client) to close
				if (re & (POLLHUP | POLLERR | POLLNVAL)){
					std::map<int, CgiFdInfo>::iterator fit = _cgiFds.find(fd);
					if (fit != _cgiFds.end()){
						int client_fd = fit->second.client_fd;

						// mark every CGI fd from this client to close
						for (std::map<int, CgiFdInfo>::iterator it = _cgiFds.begin(); it != _cgiFds.end(); it++){
							if (it->second.client_fd == client_fd)
								it->second.should_close = true;
						}

						// prepares error response for the client (if it still exists)
						std::map<int, Client>::iterator cit = _clients.find(client_fd);
						if (cit != _clients.end()){
							Client &c = cit->second;
							c.out = ResponseBuilder::buildErrorResponse(502, _config.servers[c.server_index]).toString();
							c.should_close = true;
							setPollEvents(client_fd, POLLOUT);
							
							// tries to terminate CGI process
							if (c.req_res.cgi.pid > 0){
								kill(c.req_res.cgi.pid, SIGKILL);
								waitpid(c.req_res.cgi.pid, NULL, WNOHANG);
								c.req_res.cgi.pid = -1;
							}
							c.req_res.cgi.active = false;
							c.req_res.deferred = false;
						}
					}
					continue;
				}
				if (re & POLLIN) handleCgiRead(fd);
				if (re & POLLOUT) handleCgiWrite(fd);
				continue;
			}
			
			//-----------------------------------------------------------------
			// Listen sockets
			//-----------------------------------------------------------------
			if (isListenFd(fd)){
				// Listen socket error -> remove it
				if (re & (POLLHUP | POLLERR | POLLNVAL)){
					toRemoveListens.insert(fd);
					continue;
				}
				// New connection -> accept in a loop
				if (re & POLLIN)
					acceptAll(fd);
			}

			//-----------------------------------------------------------------
			// Client sockets
			//-----------------------------------------------------------------
			else{
				// Client socket error -> remove it
				if (re & (POLLHUP | POLLERR | POLLNVAL)){
					toRemove.insert(fd);
					continue;
				}

				// Read data
				if (re & POLLIN)
					handleClientRead(fd);

				// If marked to close and no pending output -> remove
				std::map<int , Client>::iterator it = _clients.find(fd);
				if (it != _clients.end() && it->second.should_close && it->second.out.empty()){
					toRemove.insert(fd);
					continue;
				}

				// Write data
				if (re & POLLOUT)
					handleClientWrite(fd);

				// If marked to close and no pending output -> remove
				it = _clients.find(fd);
				if (it != _clients.end() && it->second.should_close && it->second.out.empty())
					toRemove.insert(fd);
			}
		}
		//-----------------------------------------------------------------
		//Remove marked fds
		//-----------------------------------------------------------------
		for (std::map<int, CgiFdInfo>::iterator it = _cgiFds.begin(); it != _cgiFds.end(); it++){
			if (it->second.should_close){
				toRemoveCgi.insert(it->first);
			}
		}

		for (std::set<int>::iterator it = toRemoveCgi.begin(); it != toRemoveCgi.end(); it++){
			int cgi_fd = *it;
			delPollFd(cgi_fd);
			close(cgi_fd);
			_cgiFds.erase(cgi_fd);
		}

		for (std::set<int>::iterator it = toRemove.begin(); it != toRemove.end(); it++)
			removeClient(*it);

		for (std::set<int>::iterator it = toRemoveListens.begin(); it != toRemoveListens.end(); it++){
			removeListen(*it);
			std::cout << "Closed listen_fd=" << *it << std::endl;

			// If there are no listen sockets left, the server cannot serve anymore.
			if (_listens.empty()){
				closeAll();
				throw std::runtime_error("all listen sockets failed");
			}
		}
	}
}

//========================================================================
// acceptAll
//========================================================================
// Accept all pending connections on listen_fd (non-blocking).
// Creates a Client, initializes state and adds its fd to poll.
void Webserv::acceptAll(int listen_fd){
	while (true){
		sockaddr_in client;
		std::memset(&client, 0, sizeof(client));
		socklen_t client_len = sizeof(client);

		int client_fd = accept(listen_fd, reinterpret_cast<sockaddr*>(&client), &client_len);
		if (client_fd < 0){
			if (errno == EINTR)
				continue;
			// Non-blocking: EAGAIN/EWOULDBLOCK -> no more clients to accept
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				return;

			std::cerr << "accept() failed on listen_fd=" << listen_fd << std::endl;
			return;
		}

		if (setNonBlocking(client_fd) < 0){
			close(client_fd);
			continue;
		}

		// Find which server_index is associated with this listen_fd
		std::map<int, ListenSocket>::iterator it =_listens.find(listen_fd);
		if (it == _listens.end()){
			close(client_fd);
			return;
		}

		// Initialize client state
		Client c;
		c.fd = client_fd;
		c.listen_fd = listen_fd;

		// IO flags
		c.want_write = false;
		c.should_close = false;

		// Parsing / keep-alive state
		c.header_parsed = false;
		c.has_completed_request = false;

		// Timeout ticks
		c.las_activity_tick = g_tick;
		c.las_write_progress_tick = g_tick;

		// Link client to the correct server config
		c.server_index = it->second.server_index;

		// Body limit (used by the parser to reject oversized requests)
		c.expected_body = _config.servers[c.server_index].client_max_body_size;

		_clients[client_fd] = c;
		addPollFd(client_fd, POLLIN); 

		std::cout << "Accepted client fd=" << client_fd << " on listen_fd=" << listen_fd << std::endl;
	}
}

//========================================================================
// setPollEvents
//========================================================================
// Update poll events (POLLIN/POLLOUT) for an fd already registered in poll.
void Webserv::setPollEvents(int fd, short event){
	for (size_t i = 0; i < _pfds.size(); i++){
		if (_pfds[i].fd == fd){
			_pfds[i].events = event;
			return ;
		}
	}
}

bool Webserv::isCgiFd(int fd) const{
	return (_cgiFds.find(fd) != _cgiFds.end());
}

void Webserv::startDeferredCgiForClient(int client_fd){
	std::map<int, Client>::iterator it = _clients.find(client_fd);
	if (it == _clients.end())
		return;
	
	Client &c = it->second;
	if (!c.req_res.deferred || !c.req_res.cgi.active)
		return;
	
	// pipes non-blocking
	if (c.req_res.cgi.stdinFd >= 0) setNonBlocking(c.req_res.cgi.stdinFd);
	if (c.req_res.cgi.stdoutFd >= 0) setNonBlocking(c.req_res.cgi.stdoutFd);

	// stdout: server reads
	if (c.req_res.cgi.stdoutFd >= 0){
		addPollFd(c.req_res.cgi.stdoutFd, POLLIN);
		_cgiFds[c.req_res.cgi.stdoutFd] = (CgiFdInfo){client_fd, false, CGI_FD_STDOUT};
	}

	// stdin: server writes body (ou fecha já se não houver body)
	if (c.req_res.cgi.stdinFd >= 0){
		if (!c.req_res.cgi.requestBody.empty()){
			addPollFd(c.req_res.cgi.stdinFd, POLLOUT);
			_cgiFds[c.req_res.cgi.stdinFd] = (CgiFdInfo){client_fd, false, CGI_FD_STDIN};
		}
		else{
			close(c.req_res.cgi.stdinFd);
			c.req_res.cgi.stdinFd = -1;
		}
	}

	// while CGI is running, we don´t want to work work with the client socket
	setPollEvents(client_fd, 0);
	c.req_res.cgiRawOutput.clear();
	c.las_activity_tick = g_tick;
}

void Webserv::handleCgiWrite(int cgi_fd){
	std::map<int, CgiFdInfo>::iterator fit = _cgiFds.find(cgi_fd);
	if (fit == _cgiFds.end())
		return ;

	if (fit->second.role != CGI_FD_STDIN)
		return;

	int client_fd = fit->second.client_fd;
	std::map<int, Client>::iterator cit = _clients.find(client_fd);
	if (cit == _clients.end()){
		fit->second.should_close = true;
		return ;
	}
	
	Client &c = cit->second;

	// done writing body -> close stdin pipe (mark for close)
	if (c.req_res.cgi.requestBody.empty()){
		fit->second.should_close = true;
		c.req_res.cgi.stdinFd = -1;
		c.las_activity_tick = g_tick;
		return;
	}

	ssize_t n = write(cgi_fd, c.req_res.cgi.requestBody.data(), c.req_res.cgi.requestBody.size());
	if (n > 0){
		c.req_res.cgi.requestBody.erase(0, n);
		c.las_activity_tick = g_tick;
		return;
	}
}

void Webserv::handleCgiRead(int cgi_fd){
	std::map<int, CgiFdInfo>::iterator fit = _cgiFds.find(cgi_fd);
	if (fit == _cgiFds.end())
		return ;

	if (fit->second.role != CGI_FD_STDOUT)
		return;

	int client_fd = fit->second.client_fd;
	std::map<int, Client>::iterator cit = _clients.find(client_fd);
	if (cit == _clients.end()){
		fit->second.should_close = true;
		return ;
	}

	Client &c = cit->second;

	char buf[4096];
	ssize_t n = read(cgi_fd, buf, sizeof(buf));

	if (n > 0){
		c.req_res.cgiRawOutput.append(buf, n);
		c.las_activity_tick = g_tick;
		return ;
	}

	if (n == 0){
		// EOF reached
		fit->second.should_close = true;
		c.req_res.cgi.stdoutFd = -1;

		// close stdin if it still exist
		if (c.req_res.cgi.stdinFd >= 0){
			std::map<int, CgiFdInfo>::iterator inIt = _cgiFds.find(c.req_res.cgi.stdinFd);
			if (inIt != _cgiFds.end())
				inIt->second.should_close = true;
			c.req_res.cgi.stdinFd = -1;
		}

		// wait for process
		if (c.req_res.cgi.pid > 0){
			waitpid(c.req_res.cgi.pid, NULL, WNOHANG);
			c.req_res.cgi.pid = -1;
		}

		// parse CGI output -> build HTTP string
		int statusCode = 200;
		std::map<std::string, std::string> headers;
		std::string body;

		CGIHandler h;
		if (!h.parseCgiOutput(c.req_res.cgiRawOutput, statusCode, headers, body)){
			c.out = ResponseBuilder::buildErrorResponse(502, _config.servers[c.server_index]).toString();
			c.should_close = true;
		}
		else{
		
		}
	}
}

/* }else{
            headers.erase("Status");

            bool hasCL = false;
            for (std::map<std::string, std::string>::const_iterator hit = headers.begin(); hit != headers.end(); ++hit){
                if (hit->first == "Content-Length" || hit->first == "content-length"){
                    hasCL = true;
                    break;
                }
            }

            std::ostringstream oss;
            oss << "HTTP/1.1 " << statusCode << "\r\n";
            for (std::map<std::string, std::string>::const_iterator hit = headers.begin(); hit != headers.end(); ++hit)
                oss << hit->first << ": " << hit->second << "\r\n";
            if (!hasCL)
                oss << "Content-Length: " << body.size() << "\r\n";
            oss << "\r\n" << body;

            c.out = oss.str();
        }

        c.req_res.deferred = false;
        c.req_res.cgi.active = false;

        // volta a responder ao cliente
        setPollEvents(client_fd, POLLOUT);
        c.las_write_progress_tick = g_tick;
        c.las_activity_tick = g_tick;
        return;
    }

    // subject: n < 0 -> não usar errno. Espera próximo poll/timeout ou POLLERR/HUP.
    return; */

//========================================================================
// handleClientRead
//========================================================================
// Reads from the socket (non-blocking), appends to client.in and tries to parse.
// When a request becomes complete:
// - calls RequestHandler (routing/files/cgi/etc.)
// - builds the response into client.out
// - switches poll events to POLLOUT
// If parsing fails -> builds an error response and marks should_close.
void Webserv::handleClientRead(int fd){
	std::map<int, Client>::iterator it = _clients.find(fd);
	if (it == _clients.end())
		return;

	Client &client = it->second;

	// If there is pending output, do not read more (simplifies the pipeline).
	if (!client.out.empty())
		return ;

	char temp[4096];

	ssize_t len = recv(fd, temp, sizeof(temp), 0);
	if (len > 0){
		client.in.append(temp, len);
		client.las_activity_tick = g_tick;
	}
	else if (len == 0){
		// Peer closed the connection
		client.should_close = true;
		return ;
	}
	else{
		// recv error: here we simply treat it as "cannot read now"
		// (ideally we would distinguish EAGAIN/EWOULDBLOCK/EINTR from fatal errors)
		return;
	}

	// Try to parse a request from the accumulated buffer
	client.parser.parseRequest(client.in, client.expected_body);
	if (client.parser.getStatus() == HttpRequest::Incomplete)
		return;

	// Complete request -> generate response
	else if (client.parser.getStatus() == HttpRequest::Complete){
		RequestResult result = RequestHandler::handleRequest(client.parser, _config.servers[client.server_index]);

		client.req_res = result;

		// Remove only the consumed request from the input (basic pipelining)
		client.in.erase(0, client.parser.getRequestSize());
		client.has_completed_request = true;

		// Reset parser for the next request
		client.parser = HttpRequest();
		
		if (client.req_res.deferred == true && client.req_res.cgi.active == true){
			startDeferredCgiForClient(fd);
			return;
		}

		client.out = result.response.toString();
		client.las_write_progress_tick = g_tick;
		setPollEvents(fd, POLLOUT);

		return ;
	}

	// Parsing error -> build error response and close
	HttpResponse err = ResponseBuilder::buildErrorResponse(client.parser.getStatusCode(), _config.servers[client.server_index]);
	client.out = err.toString();
	client.should_close = true;
	client.las_write_progress_tick = g_tick;
	client.has_completed_request = true;
	setPollEvents(fd, POLLOUT);
	client.parser = HttpRequest();
}

//========================================================================
// handleClientWrite
//========================================================================
// Sends client.out.
// When finished:
// - if should_close -> eventLoop will remove the client
// - otherwise switch back to POLLIN (keep-alive)
// - if client.in already has data (pipelining), try to parse and generate the next response
void Webserv::handleClientWrite(int fd){
	std::map<int, Client>::iterator it = _clients.find(fd);
	if (it == _clients.end())
		return;

	Client &client = it->second;

	// Nothing to send -> go back to reading
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
		// send error: keep POLLOUT and retry later
		// (ideally we would distinguish EAGAIN/EWOULDBLOCK/EINTR from fatal errors)
		setPollEvents(fd, POLLOUT);
		return ;
	}

	// Still has data to send -> keep POLLOUT
	if (!client.out.empty()){
		setPollEvents(fd, POLLOUT);
		return ;
	}

	// Response finished
	if (client.should_close)
		return;

	// Keep-alive: go back to reading
	setPollEvents(fd, POLLIN);

	// If we already have input data (pipelining), try to generate the next response immediately
	if (!client.in.empty()){
		client.parser.parseRequest(client.in, client.expected_body);
		if (client.parser.getStatus() == HttpRequest::Incomplete)
			return;
		else if (client.parser.getStatus() == HttpRequest::Complete){
			RequestResult result = RequestHandler::handleRequest(client.parser, _config.servers[client.server_index]);
			client.req_res = result;
			client.in.erase(0, client.parser.getRequestSize());
			client.parser = HttpRequest();

			if (client.req_res.deferred && client.req_res.cgi.active){
				startDeferredCgiForClient(fd);
				return;
			}

			client.out = result.response.toString();
			client.las_write_progress_tick = g_tick;
			setPollEvents(fd, POLLOUT);
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

//========================================================================
// removeClient / removeListen
//========================================================================
// Remove a client: remove from poll, close socket, erase from map.
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

// Remove a listen socket: remove from poll, close socket, erase from map.
void Webserv::removeListen(int fd){
	delPollFd(fd);
	close(fd);
	_listens.erase(fd);
}
