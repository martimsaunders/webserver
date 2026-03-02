#include "../../inc/server/Webserv.hpp"
#include "../../inc/server/Logger.hpp"
#include <iostream>
#include <ctime>
#include <sstream>

std::ofstream Logger::_file;

Logger::Logger(){}

bool Logger::init(std::string const &path){
	if (_file.is_open())
		return (true);
	_file.open(path.c_str(), std::ios::out | std::ios::app);
	if (!_file.is_open()){
		std::cerr << "Logger: cannot open file: " << path << std::endl;
		return (false);
	}
	return (true);
}

void Logger::shutdown(){
	if (_file.is_open())
		_file.close();
}

std::string Logger::timestamp(){
	std::time_t t = std::time(NULL);
	std::tm *lt = std::localtime(&t);
	if (!lt)
		return ("unknown-time");
	char buf[32];
	std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", lt);
	return std::string(buf);
}

void Logger::writeFileLine(std::string const &line){
	if (_file.is_open()){
		_file << line << std::endl;
		_file.flush();
	}
}

void Logger::response(int fd, ResponseLog const &log, int listen_fd){
	// pick color by status class (TERMINAL)
	char const *color = GREEN;
	if (log.status >= 500) color = RED;
	else if (log.status >= 400) color = YELLOW;
	else if (log.status >= 300) color = CYAN;
	else color = GREEN;

	std::cout << color << "Response fd=" << fd
			  << " \"" << log.method << " " << log.target << "\""
			  << " -> " << log.status
			  << " (" << log.bytes << " bytes)"
			  << " on listen_fd=" << listen_fd
			  << RESET << std::endl;

	// file (no colors)
	std::ostringstream oss;
	oss << timestamp()
		<< " [RESP] fd=" << fd
		<< " listen_fd=" << listen_fd
		<< " \"" << log.method << " " << log.target << "\""
		<< " -> " << log.status
		<< " bytes=" << log.bytes;
	writeFileLine(oss.str());
}

void Logger::closeClient(int fd, std::string const &reason, int listen_fd){
	// terminal
	std::cout << BOLD_RED << "Close client fd=" << fd << " reason=" << reason;
	if (listen_fd != -1)
		std::cout << " on listen_fd=" << listen_fd;
	std::cout << RESET << std::endl;

	// file
	std::ostringstream oss;
	oss << timestamp()
		<< " [CLOSE] fd=" << fd
		<< " listen_fd=" << listen_fd
		<< " reason=\"" << reason << "\"";
	writeFileLine(oss.str());
}

void Logger::accept(int client_fd, int listen_fd){
	// terminal
	std::cout << BOLD_GREEN << "Accepted client fd=" << client_fd << " on listen_fd=" << listen_fd << RESET << std::endl;

	// file
	std::ostringstream oss;
	oss << timestamp()
		<< " [ACCEPT] fd=" << client_fd
		<< " listen_fd=" << listen_fd;
	writeFileLine(oss.str());

}

void Logger::closeListen(std::string const &reason, int listen_fd){
	// terminal
	std::cout << BOLD_RED << "Close listen_fd=" << listen_fd << " reason=" << reason;
	std::cout << RESET << std::endl;

	// file
	std::ostringstream oss;
	oss << timestamp()
		<< " [CLOSE] listen_fd=" << listen_fd
		<< " reason=\"" << reason << "\"";
	writeFileLine(oss.str());
}

void Logger::listen(std::string const &host, int port, size_t server_index){
	// terminal
	std::cout << CYAN << "Listening on ";
	
	// Simple log
	if (host.empty()) std::cout << "0.0.0.0";
	else std::cout << host;
	std::cout << ":" << port << " (server_index=" << server_index << ")" << RESET << std::endl;

	// file
	std::ostringstream oss;
	oss << timestamp()
		<< " [LISTEN] " << (host.empty() ? "0.0.0.0" : host)
		<< ":" << port
		<< " server_index=" << server_index;
	writeFileLine(oss.str());
}

void Logger::shutdownMsg(std::string const &msg){
	std::cout << MAGENTA << msg << RESET << std::endl;

	std::ostringstream oss;
	oss << timestamp() << " [SHUTDOWN] " << msg << std::endl;
	writeFileLine(oss.str());
}
