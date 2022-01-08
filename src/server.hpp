#pragma once

#include <cstdint>
#include <vector>
#include <sys/epoll.h>

namespace Epoll {
	class EpollHandler {
		private:
			int32_t epfd;
			const uint32_t nthreads = 4;
			const uint32_t max_events = 32;
			const uint32_t timeout = 3000;
			std::vector<struct epoll_event> events;
		public:
			EpollHandler(void);
			bool epoll_add(int32_t fd, uint32_t events);
			bool epoll_del(int32_t fd);
			int32_t wait_events(void);
			std::vector<struct epoll_event>* get_events_vector(void);
	};
};

namespace Server {
	class TCPServer {
		private:
			// server fd
			int32_t sfd;
			// tcp backlog
			uint32_t backlog = 100;

			Epoll::EpollHandler epoll_handler;

			bool accept_new_client(void);
			void handle_client_event(int32_t conn, uint32_t events);
			void recv_message(int32_t conn);
			void close_connection(int32_t conn);
		public:
			TCPServer(void);
			~TCPServer(void);
			void listen_conns(void);
	};
};
