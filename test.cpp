#include <iostream>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <vector>
#include <list>
#include <stack>
#include <map>
#include <deque>

static void die(const char* msg) {
    std::cerr << msg << ": " << std::strerror(errno) << std::endl;
    std::exit(1);
}

static int setNonBlocking(int fd){
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1) return -1;
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) return -1;
	return 0;
}

int main() {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket < 0) die("serverSocket");
	int yes = 1;
	if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) die("setsockopt");
	sockaddr_in serverAddress;
	std::memset(&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(8080);
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(serverSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) die("bind");
	if (listen(serverSocket, 10) < 0) die("listen");
	if (setNonBlocking(serverSocket) < 0){
		close (serverSocket);
		die("non-blocking");
	}
	pollfd pfd;
	pfd.fd = serverSocket;
	pfd.events = POLLIN;
	pfd.revents = 0;
	std::vector<int> client_fds;
	while (true){
		int ret = poll(&pfd, 1, -1);
		if (ret < 0){
			if (errno == EINTR) continue;
			else die("poll");
		}
		std::cout << "revents=0x" << std::hex << pfd.revents << std::dec << std::endl;
		if (pfd.revents & POLLIN){
			while (true){
				sockaddr_in client;
				std::memset(&client, 0, sizeof(client));
				socklen_t client_len = sizeof(client);
				int client_fd = accept(serverSocket, (sockaddr*)&client, &client_len);
				if (client_fd >= 0) {
					if (setNonBlocking(client_fd) < 0){
						close(client_fd);
						continue;
					}
					client_fds.push_back(client_fd);
					std::cout << "accepted client_fd=" << client_fd << " ip=" << client.sin_addr.s_addr << " port=" << client.sin_port << std::endl;
				}
				else{
					if (errno == EAGAIN || errno == EWOULDBLOCK) break;
					else if (errno == EINTR) continue;
					die("accept");
				}
			}
		}
		if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) std::cout << "erro/hup/nval no listen fd" << std::endl;
		pfd.revents = 0;
	}
	close (serverSocket);
}

/*Obrigatório no config file

server {} múltiplos

 listen

 error_page

 client_max_body_size

 location

 allowed_methods

 return (redirect)

 root

 autoindex

 index

 upload + upload_store

 cgi por extensão*/