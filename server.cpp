#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/timerfd.h>
#include <netinet/tcp.h>


#include "common.h"

#include <bits/stdc++.h>

using namespace std;

#define MAX_CONNECTIONS 32
#define IP "127.0.0.1"

void send_packet(int sockfd, const char command[15], const char topic[50], int sf, struct udp_packet udp_message) {
	struct homework_prot packet;
	memset(&packet, 0, sizeof(packet));

	strcpy(packet.command, command);
	strcpy(packet.topic, topic);
	packet.sf = sf;
	memcpy(&(packet.udp_message), &udp_message, sizeof(udp_packet));

	send_all(sockfd, &packet, sizeof(packet));
}

int main(int argc, char *argv[]) {

	setvbuf(stdout, NULL, _IONBF, MAX_MESSAGE_LENGTH);

	set<string> clients;
	map<int, string> get_client_id;
	map<string, int> get_client_fd;

	set<string> user_offline;
	map<string, vector<struct udp_packet>> offline_users_storage;

	/** store all sf of users using the key {user, topic}*/
	map<pair<string, string>, int> clients_sf;


	/**
	 * Key -> Topic
	 * Value -> An array containing a pair: {used_id, sf}
	*/
	map<string, vector<pair<string, int>>> subscribers;

	uint16_t port;
	int rc = sscanf(argv[1], "%hu", &port);
	DIE(rc != 1, "Given port is invalid\n");

	// ---- set the udp server socket ---- //
	int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(udp_socket < 0, "udp_socket");

	int enable = 1;
	if (setsockopt(udp_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
	perror("setsockopt(SO_REUSEADDR) failed");

	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	rc = inet_pton(AF_INET, "127.0.0.1", &(servaddr.sin_addr.s_addr));
	DIE(rc <= 0, "inet_pton");

	rc = bind(udp_socket, (const struct sockaddr *) &servaddr, sizeof(servaddr));
	DIE(rc < 0, "bind");

	// ---- set the tcp server socket ---- //
	int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
	DIE(tcp_socket < 0, "tcp_socket");

	if (setsockopt(tcp_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
	perror("setsockopt(SO_REUSEADDR) failed");

	if (setsockopt(tcp_socket, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int)) < 0)
	perror("setsockopt(TCP_NODELAY) failed");

	rc = bind(tcp_socket, (const struct sockaddr *) &servaddr, sizeof(servaddr));
	DIE(rc < 0, "bind");

	rc = listen(tcp_socket, MAX_CONNECTIONS - 3);
	DIE(rc < 0, "listen");

	// ---- initialize poll array ---- //
	vector<struct pollfd> poll_fds(3);

	poll_fds[0].fd = STDIN_FILENO;
	poll_fds[0].events = POLLIN;

	poll_fds[1].fd = udp_socket;
	poll_fds[1].events = POLLIN;

	poll_fds[2].fd = tcp_socket;
	poll_fds[2].events = POLLIN;

	// ---- start the server ----
	bool server_running = true;
	while (server_running) {

		rc = poll(poll_fds.data(), poll_fds.size(), -1);
		DIE(rc < 0, "poll");

		for (int i = 0; i < poll_fds.size() && server_running; i++) {
			if (poll_fds[i].revents & POLLIN) {
				// ---- Get packet from udp client ---- //
				if (poll_fds[i].fd == udp_socket) {

					struct udp_packet udp_packet;

					struct sockaddr_in client_addr;
					memset(&client_addr, 0, sizeof(client_addr));
					socklen_t clen = sizeof(client_addr);

					int rc = recvfrom(udp_socket, &udp_packet, sizeof(udp_packet), 0,
											(struct sockaddr *) &client_addr, &clen);

					string topic;
					topic.append(udp_packet.topic);					

					if (subscribers.find(topic) == subscribers.end()) {
						continue;
					}

					char address[50];
					char port[10];
					inet_ntop(AF_INET, &(client_addr.sin_addr), address, INET_ADDRSTRLEN);
					sprintf(port, "%hu", ntohs(client_addr.sin_port));
					strcat(address + strlen(address), ":");
					strcat(address + strlen(address), port);

					for (auto subscriber : subscribers[topic]) {
						string subscriber_id = subscriber.first;
						if (get_client_fd[subscriber_id] != 0) {
							send_packet(get_client_fd[subscriber_id], "news", address, -1, udp_packet);
						} else if (clients_sf[{subscriber_id, topic}] == 1) {
							offline_users_storage[subscriber_id].push_back(udp_packet);
						}
					}

				// ---- handle new connection from tcp client ---- //
				} else if (poll_fds[i].fd == tcp_socket) {

					struct sockaddr_in cli_addr;
					socklen_t cli_len = sizeof(cli_addr);
					int newsockfd = accept(tcp_socket, (struct sockaddr *) &cli_addr, &cli_len);

					struct homework_prot p;
					memset(&p, 0, sizeof(p));
					recv_all(newsockfd,(void *) &p, sizeof(p));

					string client;
					client.append(p.command);

					bool is_registered = (user_offline.find(client) != user_offline.end());
					bool new_user = (clients.find(client) == clients.end());

					if (get_client_fd.find(client) != get_client_fd.end() &&
						get_client_fd[client] != 0) {

						printf("Client %s already connected.\n", p.command);

						struct udp_packet p;
						send_packet(newsockfd, "refuse", "", -1, p);

						close(newsockfd);

						continue;
					}


					// ---- connect the user back to the server ---- //
					get_client_id[newsockfd] = client;
					get_client_fd[client] = newsockfd;
					clients.insert(client);

					struct pollfd new_client;
					new_client.fd = newsockfd;
					new_client.events = POLLIN;

					poll_fds.push_back(new_client);
					
					// ---- if the user is registered send all the news he missed ----
					if (is_registered) {
						user_offline.erase(client);
						for (auto news : offline_users_storage[client]) {
							send_packet(newsockfd, "news", "", -1, news);
						}
						offline_users_storage[client].clear();
					}

					printf("New client %s connected from %s:%hu.\n", p.command,
					inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

				// ---- get command from console ---- //
				} else if (poll_fds[i].fd == STDIN_FILENO) {
					char command[MAX_MESSAGE_LENGTH];
					fgets(command, sizeof(command), stdin);

					if (strncmp(command, "exit", 4) == 0) {

						server_running = false;

						struct udp_packet p; // just a filler for the function to work
						for (int j = 3 ;j < poll_fds.size(); j++) {
							send_packet(poll_fds[j].fd, "exit", "", -1, p);
						}

					} else {
						printf("Use the 'exit' command to close the server.\n");
					}

				// ---- a client sends data to the server ---- //
				} else {
					struct homework_prot packet;
					memset(&packet, 0, sizeof(packet));
					recv(poll_fds[i].fd, &packet, sizeof(packet), 0);

					if (strncmp(packet.command, "subscribe", 9) == 0) {

						string client_id = get_client_id[poll_fds[i].fd];

						string topic;
						topic.append(packet.topic);

						subscribers[topic].push_back({client_id, packet.sf});
						clients_sf[{client_id, topic}] = packet.sf;

					} else if (strncmp(packet.command, "unsubscribe", 11) == 0) {

						string client_id = get_client_id[poll_fds[i].fd];

						for (auto topic : subscribers) {
							vector<pair<string, int>> topic_subscribers = topic.second;
							for (auto it = topic_subscribers.begin(); it != topic_subscribers.end(); it++) {
								if ((*it).first == client_id) {
									topic_subscribers.erase(it);
									clients_sf.erase({client_id, topic.first});
									break;
								}
							}
							subscribers[topic.first] = topic_subscribers;
						}

					} else if (strncmp(packet.command, "exit", 4) == 0) {
						for (auto it = poll_fds.begin(); it != poll_fds.end(); it++) {
							if ((*it).fd == poll_fds[i].fd) {
								string client_id = get_client_id[poll_fds[i].fd];
								cout << "Client " << client_id << " disconnected.\n";
								user_offline.insert(client_id);
								get_client_fd[client_id] = 0;
								poll_fds.erase(it);
								break;
							}
						}
					}
				}
			}
		}
	}

	for (int i = 1; i < poll_fds.size(); i++) {
		close(poll_fds[i].fd);
	}

	return 0;

}
