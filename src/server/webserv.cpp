#include "../../inc/server/Webserv.hpp"
#include "../../inc/request/CGIHandler.hpp"
#include "../../inc/request/RequestHandler.hpp"
#include "../../inc/response/HttpResponse.hpp"
#include "../../inc/response/ResponseBuilder.hpp"
#include "../../inc/server/Logger.hpp"
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

#define WRITE_STALL_SECONDS 10
#define CONNECT_IDLE_SECONDS 15
#define HEADER_TIMEOUT_SECONDS 10
#define KEEPALIVE_SECONDS 30
#define CGI_TIMEOUT_SECONDS 10

static void terminateClientCgi(Client &c);

//========================================================================
// Global loop state (signals + logical "tick")
//========================================================================
// Flag set by signal handlers to request a graceful shutdown.
static volatile sig_atomic_t g_stop = 0;

/* // Loop "tick" (increments every iteration). Used for simple timeouts.
static unsigned long g_tick = 0; */

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

static std::string nowHttpDate()
{
	char buf[128];
	std::time_t t = std::time(NULL);
	std::tm* gmt = std::gmtime(&t);
	if (!gmt)
		return "";
	if (std::strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", gmt) == 0)
		return "";
	return std::string(buf);
}

static bool iequalsAscii(const std::string& a, const std::string& b)
{
	if (a.size() != b.size())
		return false;
	for (size_t i = 0; i < a.size(); ++i)
	{
		unsigned char ca = static_cast<unsigned char>(a[i]);
		unsigned char cb = static_cast<unsigned char>(b[i]);
		if (ca >= 'A' && ca <= 'Z') ca = static_cast<unsigned char>(ca - 'A' + 'a');
		if (cb >= 'A' && cb <= 'Z') cb = static_cast<unsigned char>(cb - 'A' + 'a');
		if (ca != cb)
			return false;
	}
	return true;
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

	Logger::init("logs/webserv.log");
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
		Logger::listen(sc.host, sc.port, i);
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

		if (listen(fd, SOMAXCONN) < 0){
			close(fd);
			fd = -1;
			continue;
		}

		// Success
		break;
	}

	freeaddrinfo(res);

	if (fd < 0)
		throw std::runtime_error("failed to create listen socket");

	return (fd);
}

// Close all sockets (clients + listens) and clear internal structures.
void Webserv::closeAll(){
	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); it++){
		delPollFd(it->first);
		close(it->first);
	}	
	_clients.clear();

	for (std::map<int, ListenSocket>::iterator it = _listens.begin(); it != _listens.end(); it++){
		delPollFd(it->first);
		close(it->first);
	}
	_listens.clear();

	// close CGI fds too
	for (std::map<int, CgiFdInfo>::iterator it = _cgiFds.begin(); it != _cgiFds.end(); it++){
		delPollFd(it->first);
		close(it->first);
	}
	_cgiFds.clear();

	_pfds.clear();
}

static void killAllCgi(std::map<int, Client> &clients){
	for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); it++){
		pid_t pid = it->second.req_res.cgi.pid;
		if (pid > 0)
			kill(pid, SIGKILL);
	}
	for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); it++){
		pid_t pid = it->second.req_res.cgi.pid;
		if (pid > 0){
			pid_t r = waitpid(pid, NULL, WNOHANG);
			if (r == pid || r < 0)
				it->second.req_res.cgi.pid = -1;
		}
	}
}

void Webserv::markReapedPid(pid_t pid)
{
	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); it++)
	{
		Client &c = it->second;
		if (c.req_res.cgi.pid == pid){
			c.req_res.cgi.active = false;
			c.req_res.deferred = false;
			c.req_res.cgi.pid = -1;
			int client_fd = it->first;
			for (std::map<int, CgiFdInfo>::iterator fit = _cgiFds.begin(); fit != _cgiFds.end(); ++fit)
				if (fit->second.client_fd == client_fd)
					fit->second.should_close = true;
			return;
		}
	}
}

void Webserv::reapChildrenNoHang(){
	int status;
	while (true)
	{
		pid_t r = waitpid(-1, &status, WNOHANG);
		if (r > 0){
			markReapedPid(r);
			continue;
		}
		break;
	}
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
		// global non blocking reap
		reapChildrenNoHang();

		// Graceful shutdown via signals.
		if (g_stop){
			killAllCgi(_clients);
			closeAll();
			Logger::shutdownMsg("Shutting down server peacefully...");
			return;
		}
		if (_pfds.empty())
			throw std::runtime_error("no fds to poll()");

		// poll timeout 1000ms: wake up to update timeouts and react to g_stop
		int rc = poll(&_pfds[0], _pfds.size(), 1000);
		time_t now = std::time(NULL);
		if (now == (time_t)-1)
			now = 0;

		if (rc < 0){
			// poll interrupted by signal -> restart loop
			if (errno == EINTR)
				continue;
			throw std::runtime_error("poll() failed");
		}

		// Sets of fds to remove (remove outside loops to avoid invalidating iterators).
		// Sets of fds to remove with reason (remove outside loops to avoid invalidating iterators).
		std::set<int> toRemoveCgi;
		std::map<int, std::string> toRemove;
		std::map<int, std::string> toRemoveListens;

		struct InsertReason{
			static void add(std::map<int, std::string> &m, int fd, std::string const &reason){
				if (m.find(fd) == m.end())
					m[fd] = reason;
			}
		};

		//-----------------------------------------------------------------
		// Per-client timeouts (tick-based)
		//-----------------------------------------------------------------
		for (std::map<int, Client>::iterator cit = _clients.begin(); cit != _clients.end(); cit++){
			Client &c = cit->second;

			// While CGI is running, uses its own timeout
			if (c.req_res.deferred && c.req_res.cgi.active){
				if ((now - c.las_activity_tick) > CGI_TIMEOUT_SECONDS){
					// close/mark all CGI fds for this client
					for (std::map<int, CgiFdInfo>::iterator it = _cgiFds.begin(); it != _cgiFds.end(); it++){
						if (it->second.client_fd == cit->first)
							it->second.should_close = true;
					}

					// terminate CGI (non-blocking, global reap handles remaining)
					terminateClientCgi(c);
					
					// prepare HTTP 504 response and let poll flush it
					c.out = ResponseBuilder::buildErrorResponse(504, _config.servers[c.server_index]).toString();
					c.log.status = 504;
					c.log.bytes = c.out.size();
					c.should_close = true;

					setPollEvents(cit->first, POLLOUT);
					c.las_activity_tick = now;
					c.las_write_progress_tick = now;
				}	
				continue;
			}

			// If we have been writing for too long without progress -> remove
			if (!c.out.empty() && (now - c.las_write_progress_tick) > WRITE_STALL_SECONDS){
				InsertReason::add(toRemove, cit->first, "write stall timeout");
				continue;
			}

			// If we received something (in not empty) but have no output yet,
			// treat as "waiting for full header/request".
			if (!c.in.empty() && c.out.empty()){
				if ((now - c.las_activity_tick) > HEADER_TIMEOUT_SECONDS)
					InsertReason::add(toRemove, cit->first, "header timeout");
				continue;
			}

			// If both buffers are empty, distinguish:
			// - no request completed yet -> idle connect timeout
			// - at least one request completed -> keep-alive timeout
			if (c.in.empty() && c.out.empty()){
				if (!c.has_completed_request){
					if ((now - c.las_activity_tick) > CONNECT_IDLE_SECONDS)
						InsertReason::add(toRemove, cit->first, "connect idle timeout");
				}
				else{
					if ((now - c.las_activity_tick) > KEEPALIVE_SECONDS)
						InsertReason::add(toRemove, cit->first, "keep-alive timeout");
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
				// Fatal CGI fd errors: mark this fd (and sibling CGI fds) to close.
				// Note: POLLHUP is expected on pipe EOF and is handled below via read().
				if (re & (POLLERR | POLLNVAL)){
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
							
							// terminate CGI (non-blocking, global reap handles remaining)
							terminateClientCgi(c);
						}
					}
					continue;
				}
				if (re & (POLLIN | POLLHUP)) handleCgiRead(fd);
				if (re & POLLOUT) handleCgiWrite(fd);
				continue;
			}
			
			//-----------------------------------------------------------------
			// Listen sockets
			//-----------------------------------------------------------------
			if (isListenFd(fd)){
				// Listen socket error -> remove it
				if (re & (POLLHUP | POLLERR | POLLNVAL)){
					InsertReason::add(toRemoveListens, fd, "listen socket error (HUP/ERR/NVAL)");
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
					InsertReason::add(toRemove, fd, "client socket error (HUP/ERR/NVAL)");
					continue;
				}

				// Read data
				if (re & POLLIN)
					handleClientRead(fd);

				// If marked to close and no pending output -> remove
				std::map<int , Client>::iterator it = _clients.find(fd);
				if (it != _clients.end() && it->second.should_close && it->second.out.empty()){
					InsertReason::add(toRemove, fd, "client marked close");
					continue;
				}

				// Write data
				if (re & POLLOUT)
					handleClientWrite(fd);

				// If marked to close and no pending output -> remove
				it = _clients.find(fd);
				if (it != _clients.end() && it->second.should_close && it->second.out.empty())
					InsertReason::add(toRemove, fd, "client marked close");
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

		for (std::map<int, std::string>::iterator it = toRemove.begin(); it != toRemove.end(); it++)
			removeClient(it->first, it->second);

		for (std::map<int, std::string>::iterator it = toRemoveListens.begin(); it != toRemoveListens.end(); it++){
			Logger::closeListen(it->second, it->first);
			removeListen(it->first);

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
		time_t now = std::time(NULL);
		if (now == (time_t)-1) now = 0;
		c.las_activity_tick = now;
		c.las_write_progress_tick = now;

		// Link client to the correct server config
		c.server_index = it->second.server_index;

		// Body limit (used by the parser to reject oversized requests)
		c.expected_body = _config.servers[c.server_index].client_max_body_size;

		_clients[client_fd] = c;
		addPollFd(client_fd, POLLIN); 

		Logger::accept(client_fd, listen_fd);
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
			safeCloseFd(c.req_res.cgi.stdinFd);
		}
	}

	// while CGI is running, use POLLIN to detect disconnect
	setPollEvents(client_fd, POLLIN);
	c.req_res.cgiRawOutput.clear();
	time_t now = std::time(NULL);
	if (now == (time_t)-1) now = 0;
	c.las_activity_tick = now;
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
		time_t now = std::time(NULL);
		if (now == (time_t)-1) now = 0;
		c.las_activity_tick = now;
		return;
	}

	ssize_t n = write(cgi_fd, c.req_res.cgi.requestBody.data(), c.req_res.cgi.requestBody.size());
	if (n > 0){
		c.req_res.cgi.requestBody.erase(0, n);
		time_t now = std::time(NULL);
		if (now == (time_t)-1) now = 0;
		c.las_activity_tick = now;
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
		time_t now = std::time(NULL);
		if (now == (time_t)-1) now = 0;
		c.las_activity_tick = now;
		return ;
	}

	if (n == 0){
		// EOF reached
		fit->second.should_close = true;

		// close stdin if it still exist
		if (c.req_res.cgi.stdinFd >= 0){
			std::map<int, CgiFdInfo>::iterator inIt = _cgiFds.find(c.req_res.cgi.stdinFd);
			if (inIt != _cgiFds.end())
				inIt->second.should_close = true;
			c.req_res.cgi.stdinFd = -1;
		}

		// parse CGI output -> build HTTP string
		int statusCode = 200;
		std::map<std::string, std::string> headers;
		std::string body;
		
		CGIHandler h;
		if (!h.parseCgiOutput(c.req_res.cgiRawOutput, statusCode, headers, body)){
			c.out = ResponseBuilder::buildErrorResponse(502, _config.servers[c.server_index]).toString();
			c.should_close = true;
			c.log.status = 502;
			c.log.bytes = c.out.size();
		}
		else{
			// Determine final Content-Type from CGI output (fallback to text/html).
			std::string contentType = "text/html";
			for (std::map<std::string, std::string>::const_iterator hit = headers.begin(); hit != headers.end(); ++hit){
				if (iequalsAscii(hit->first, "Content-Type")){
					contentType = hit->second;
					break;
				}
			}

			// Build HTTP response headers in canonical order.
			std::ostringstream oss;
			oss << "HTTP/1.1 " << statusCode << " " << ResponseBuilder::resolveReasonPhrase(statusCode) << "\r\n";
			oss << "Date: " << nowHttpDate() << "\r\n";
			oss << "Server: webserv\r\n";
			oss << "Content-Length: " << body.size() << "\r\n";
			oss << "Content-Type: " << contentType << "\r\n";
			oss << "Connection: " << (c.should_close ? "close" : "keep-alive") << "\r\n";

			// Append CGI-specific headers, excluding standard managed headers.
			for (std::map<std::string, std::string>::const_iterator hit = headers.begin(); hit != headers.end(); ++hit){
				if (iequalsAscii(hit->first, "Status")
					|| iequalsAscii(hit->first, "Date")
					|| iequalsAscii(hit->first, "Server")
					|| iequalsAscii(hit->first, "Content-Length")
					|| iequalsAscii(hit->first, "Content-Type")
					|| iequalsAscii(hit->first, "Connection"))
					continue;
				oss << hit->first << ": " << hit->second << "\r\n";
			}
			oss << "\r\n" << body;

			c.out = oss.str();

			c.log.status = statusCode;
			c.log.bytes = c.out.size();
		}

		c.req_res.deferred = false;
		c.req_res.cgi.active = false;

		// Client will respond again
		setPollEvents(client_fd, POLLOUT);
		time_t now = std::time(NULL);
		if (now == (time_t)-1) now = 0;
		c.las_write_progress_tick = now;
		c.las_activity_tick = now;
		return;
	}

	// subject: n < 0 -> não usar errno. Espera próximo poll/timeout ou POLLERR/HUP.
	return;
}

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

	// If CGI is running, don't process HTTP
	// We only want to know if client closed (recv == 0)
	if (client.req_res.deferred && client.req_res.cgi.active){
		char tmp[1];
		ssize_t r = recv(fd, tmp, sizeof(tmp), MSG_PEEK);
		if (r == 0){
			client.should_close = true; // event loop removes later

			for (std::map<int, CgiFdInfo>::iterator fit = _cgiFds.begin(); fit != _cgiFds.end(); ++fit)
				if (fit->second.client_fd == fd)
					fit->second.should_close = true;

			terminateClientCgi(client);
		}
		return ;
	}

	// If there is pending output, do not read more (simplifies the pipeline).
	if (!client.out.empty())
		return ;

	char temp[4096];

	ssize_t len = recv(fd, temp, sizeof(temp), 0);
	if (len > 0){
		client.in.append(temp, len);
		time_t now = std::time(NULL);
		if (now == (time_t)-1) now = 0;
		client.las_activity_tick = now;
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
		// Save request info BEFORE resetting parser
		client.log.method = client.parser.getMethod();
		client.log.target = client.parser.getUri();

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

		// status/bytes of the RESPONSE (not parser)
		client.log.status = result.response.getStatusCode();
		client.log.bytes = client.out.size();

		time_t now = std::time(NULL);
		if (now == (time_t)-1) now = 0;
		client.las_activity_tick = now;
		setPollEvents(fd, POLLOUT);

		return ;
	}

	// Parsing error -> build error response and close
	HttpResponse err = ResponseBuilder::buildErrorResponse(client.parser.getStatusCode(), _config.servers[client.server_index]);
	client.log.method = client.parser.getMethod();
	client.log.target = client.parser.getUri();
	client.out = err.toString();
	client.log.status = err.getStatusCode();
	client.log.bytes = client.out.size();
	client.should_close = true;
	time_t now = std::time(NULL);
	if (now == (time_t)-1) now = 0;
	client.las_write_progress_tick = now;
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
		time_t now = std::time(NULL);
		if (now == (time_t)-1) now = 0;
		client.las_write_progress_tick = now;
		client.las_activity_tick = now;
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

	// Response finished -> log now (sent complete)
	Logger::response(fd, client.log, client.listen_fd);

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
			// Save request info BEFORE resetting parser
			client.log.method = client.parser.getMethod();
			client.log.target = client.parser.getUri();

			RequestResult result = RequestHandler::handleRequest(client.parser, _config.servers[client.server_index]);
			client.req_res = result;
			client.in.erase(0, client.parser.getRequestSize());
			client.parser = HttpRequest();

			if (client.req_res.deferred && client.req_res.cgi.active){
				startDeferredCgiForClient(fd);
				return;
			}

			client.out = result.response.toString();
			client.log.status = result.response.getStatusCode();
			client.log.bytes = client.out.size();

			time_t now = std::time(NULL);
			if (now == (time_t)-1) now = 0;
			client.las_write_progress_tick = now;
			setPollEvents(fd, POLLOUT);
			return ;
		}
		HttpResponse err = ResponseBuilder::buildErrorResponse(client.parser.getStatusCode(), _config.servers[client.server_index]);
		client.log.method = client.parser.getMethod();
		client.log.target = client.parser.getUri();

		client.out = err.toString();
		client.log.status = err.getStatusCode();
		client.log.bytes = client.out.size();

		client.should_close = true;
		time_t now = std::time(NULL);
		if (now == (time_t)-1) now = 0;
		client.las_write_progress_tick = now;
		setPollEvents(fd, POLLOUT);
		client.parser = HttpRequest();
	}
}

//========================================================================
// removeClient / removeListen
//========================================================================

static void terminateClientCgi(Client &c){
	// Force kill (global reaper handles the rest later)
	if (c.req_res.cgi.pid > 0){
		kill(c.req_res.cgi.pid, SIGKILL);
		// r == 0 -> still running, keep pid for global reap
	}

	c.req_res.cgi.active = false;
	c.req_res.deferred = false;
}

// Remove a client: remove from poll, close socket, erase from map.
void Webserv::removeClient(int fd, std::string const &reason){
	int listen_fd = -1;

	// Always mark any CGI fds linked to this client for removal from poll/map
	for (std::map<int, CgiFdInfo>::iterator fit = _cgiFds.begin(); fit != _cgiFds.end(); fit++){
		if (fit->second.client_fd == fd)
			fit->second.should_close = true;
	}

	std::map<int, Client>::iterator it = _clients.find(fd);
	if (it != _clients.end()){
		listen_fd = it->second.listen_fd;

		// Ensure any CGI owned by this client is terminated and reaped
		terminateClientCgi(it->second);
	}
		

	delPollFd(fd);
	close(fd);

	if (it != _clients.end())
		_clients.erase(it);

	Logger::closeClient(fd, reason, listen_fd);
}

// Remove a listen socket: remove from poll, close socket, erase from map.
void Webserv::removeListen(int fd){
	delPollFd(fd);
	close(fd);
	_listens.erase(fd);
}
