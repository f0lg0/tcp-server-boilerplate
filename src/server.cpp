#include "server.hpp"

#include <iostream>
#include <cstdint>
#include <unistd.h>
#include <algorithm>

#include <sys/epoll.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>

Epoll::EpollHandler::EpollHandler(void) {
	this->epfd = epoll_create(this->nthreads);
	if (this->epfd < 0) {
		std::cerr << "[!] Failed to create epoll, exiting..." << std::endl;
		exit(1);
	}
	this->events.resize(this->max_events);
}

bool Epoll::EpollHandler::epoll_add(int32_t fd, uint32_t events) {
	struct epoll_event event = {0};
	event.events = events;
	event.data.fd = fd;

	if (epoll_ctl(this->epfd, EPOLL_CTL_ADD, fd, &event) < 0) return false;

	return true;
}

bool Epoll::EpollHandler::epoll_del(int32_t fd) {
	if (epoll_ctl(this->epfd, EPOLL_CTL_DEL, fd, NULL) < 0) return false;
	return true;
}

int32_t Epoll::EpollHandler::wait_events(void) {
	return epoll_wait(this->epfd, &(this->events[0]), this->max_events, this->timeout);
}

std::vector<struct epoll_event>* Epoll::EpollHandler::get_events_vector(void) {
	return &(this->events);
}

Server::TCPServer::TCPServer(void) {
	this->sfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
	if (this->sfd == -1) {
		std::cerr << "[!] Erro while creating socket, exiting..." << std::endl;
		exit(1);
	}

	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr("0.0.0.0");
	address.sin_port = htons(strtol("1234", NULL, 10));

	if (bind(this->sfd, (struct sockaddr*) &address, sizeof(address)) < 0) {
		std::cerr << "[!] Erro while binding socket to specified address (0.0.0.0:1234), exiting..." << std::endl;
		exit(1);
	}
}

Server::TCPServer::~TCPServer(void) {
	close(this->sfd);
}

// get sockaddr ip_v4
static void* get_in_addr(struct sockaddr *sa) {
	return &(((struct sockaddr_in*)sa)->sin_addr);
}

// get sockaddr port
static unsigned short get_in_port(struct sockaddr *sa) {
	return ((struct sockaddr_in*)sa)->sin_port;
}

bool Server::TCPServer::accept_new_client(void) {
	sockaddr_storage clients;
	socklen_t clients_size = sizeof(clients);

	int32_t conn = accept(this->sfd, (sockaddr*) &clients, &clients_size);

	char conn_addr[INET_ADDRSTRLEN];
	inet_ntop(clients.ss_family, get_in_addr((struct sockaddr*)&clients), conn_addr, sizeof(conn_addr));

	std::cout << "[+] Got connection from: " << conn_addr << ":" << get_in_port((struct sockaddr*)&clients) << " --> fd: " << conn << std::endl;

	// tell epoll to monitor client fd
	return this->epoll_handler.epoll_add(conn, EPOLLIN | EPOLLPRI);
}

void Server::TCPServer::close_connection(int32_t conn) {
	std::cout << "[!] Client with fd: " << conn << " has closed its connection" << std::endl;
	this->epoll_handler.epoll_del(conn);
	close(conn);
}

void Server::TCPServer::recv_message(int32_t conn) {
	char len_str[4];
	int32_t recvd = recv(conn, len_str, sizeof(uint32_t), 0);

	if (recvd == 0) {
		// client disconnected
		this->close_connection(conn);
		return;
	}
	
	if (recvd == sizeof(uint32_t)) {
		uint32_t len = atoi(len_str);
		std::vector<char> buffer(len + 1, 0x00);

		uint32_t bytes_read = 0;
		bool streaming_err = false;

		while (bytes_read < len) {
			int32_t recvd_bytes = recv(conn, &buffer[bytes_read], len - bytes_read, 0);

			if(recvd_bytes != -1) {
				bytes_read += recvd_bytes;
			} else {
				streaming_err = true;
				break;
			}
		}

		if (streaming_err) {
			std::cerr << "[!] Errors while receiving message from fd: " << conn << std::endl;
			this->close_connection(conn);
			return;
		}

		// buffer now contains the complete message
		std::string payload = std::string(buffer.begin(), buffer.end());
		// removing the null byte
		payload.erase(std::find(payload.begin(), payload.end(), '\0'), payload.end()); 
		// printing to screen
		std::cout << "[+] Received: " << payload << std::endl;
	}

}

void Server::TCPServer::handle_client_event(int32_t conn, uint32_t events) {
	const uint32_t err_mask = EPOLLERR | EPOLLHUP;

	if (events & err_mask) {
		this->close_connection(conn);
		return;
	} 

	// stream incoming message
	this->recv_message(conn);
}

void Server::TCPServer::listen_conns(void) {
	if (listen(this->sfd, this->backlog) < 0) {
		std::cerr << "[!] Error while listening on specified address (0.0.0.0:1234), exiting..." << std::endl;
		exit(1);
	}

	// add sock_fd to epoll monitoring
	if (!(this->epoll_handler.epoll_add(this->sfd, EPOLLIN | EPOLLPRI))) {
		std::cerr << "[!] Failed to add fd to epoll, exiting..." << std::endl;
		exit(1);
	}

	std::cout << "[+] Listening on 0.0.0.0:1234" << std::endl;

	bool err = false;
	int32_t events_ret = 0x00;
	while (1) {
		events_ret = this->epoll_handler.wait_events();

		if (events_ret == 0) {
			continue;
		}

		if (events_ret == -1) {
			if (errno == EINTR) continue;

			std::cerr << "[!] epoll error!" << std::endl;
			err = true;
			break;
		}

		std::vector<struct epoll_event> events = *(this->epoll_handler.get_events_vector());
		for (uint32_t i = 0; i < events_ret; i++) {
			if (events[i].data.fd == this->sfd) {
				// new client is connecting to us
				if (!(this->accept_new_client())) {
					std::cerr << "[!] Error while accepting client connection!" << std::endl;
					err = true;
					break;
				}

				continue;
			}

			// we have event(s) from client
			this->handle_client_event(events[i].data.fd, events[i].events);
		}

		if (err) break;
	}
}

int main() {
	Server::TCPServer server;
	server.listen_conns();

	return 0;
}