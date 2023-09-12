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

#include "common.h"

#include <bits/stdc++.h>

using namespace std;

void print_udp(int sockfd, struct udp_packet p) {
	
	switch(p.tip_date) {
		case 0: {
			uint32_t int_number;
			memcpy(&int_number, p.continut + 1, 4);
			int_number = ntohl(int_number);
			if (p.continut[0] == 0) {
				printf("%s - INT - %d\n", p.topic, int_number);
			} else {
				printf("%s - INT - -%d\n", p.topic, int_number);
			}
			break;
		}
		case 1: {
			float short_float;
			uint16_t short_number;
			memcpy(&short_number, p.continut, 2);
			short_number = ntohs(short_number);
			short_float = 1. * short_number / 100;
			printf("%s - SHORT_REAL - %.2f\n", p.topic, short_float);
			break;
		}
		case 2: {
			uint32_t int_number;
			float float_number;
			memcpy(&int_number, p.continut + 1, 4);
			int_number = ntohl(int_number);
			
			uint8_t power = p.continut[5];
			float_number = (1. * int_number) / pow(10, power);

			if (p.continut[0] == 0) {
				printf("%s - FLOAT - %f\n", p.topic, float_number);
			} else {
				printf("%s - FLOAT - -%f\n", p.topic, float_number);
			}
			break;
		}
		case 3: {
			cout << p.topic << " - " << "STRING" << " - " << p.continut << '\n';
			break;
		}
		default:
		exit(0);
		break;
	}
}

int main(int argc, char *argv[]) {

	setvbuf(stdout, NULL, _IONBF, MAX_MESSAGE_LENGTH);

	uint16_t port;
	int rc = sscanf(argv[3], "%hu", &port);
	DIE(rc != 1, "Given port is invalid\n");

	char client_id[10];
	memcpy(client_id, argv[1], sizeof(client_id));

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	struct sockaddr_in serv_addr;
	socklen_t socket_len = sizeof(struct sockaddr_in);

	memset(&serv_addr, 0, socket_len);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	rc = inet_pton(AF_INET, argv[2], &serv_addr.sin_addr.s_addr);
	DIE(rc <= 0, "inet_pton");

	rc = connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
	DIE(rc < 0, "connect");

	struct homework_prot p;
	memset(&p, 0, sizeof(p));
	strcpy(p.command, client_id);
	send_all(sockfd, (void *) &p, sizeof(p));

	char command[MAX_MESSAGE_LENGTH];
	memset(command, 0, MAX_MESSAGE_LENGTH);

	struct pollfd fds[2];
	fds[0].fd = STDIN_FILENO;
	fds[0].events = POLLIN;

	fds[1].fd = sockfd;
	fds[1].events = POLLIN;

	struct homework_prot packet;


	bool client_running = true;
	while (client_running) {

		poll(fds, 2, -1);

		for (int i = 0; i < 2 && client_running; i++) {
			if (fds[i].revents & POLLIN) {
				if (fds[i].fd == STDIN_FILENO) {
					fgets(command, sizeof(command), stdin);

					char command_type[15];
					memset(command_type, 0, sizeof(command_type));
					char topic[50];
					memset(topic, 0, sizeof(topic));
					int sf = -1;
					int nr_args = sscanf(command, "%s %s %d", command_type, topic, &sf);

					memset(&packet, 0, sizeof(packet));
					strcpy(packet.command, command_type);
					strcpy(packet.topic, topic);
					packet.sf = sf;

					if (strncmp(command_type, "subscribe", 9) == 0 && nr_args == 3) {

						if (sf != 0 && sf != 1) {
							printf("Invalid command.\n");
							continue;
						}

						send_all(sockfd, (void *) &packet, sizeof(packet));
						printf("Subscribed to topic.\n");
						
					} else if (strncmp(command_type, "unsubscribe", 11) == 0 && nr_args == 2) {
						send_all(sockfd, (void *) &packet, sizeof(packet));
						printf("Unsubscribed to topic.\n");
					} else if (strncmp(command_type, "exit", 4) == 0 && nr_args == 1) {
						
						send_all(sockfd, (void *) &packet, sizeof(packet));
						client_running = false;
					} else {
						printf("Invalid command.\n");
					}
					
				} else if (fds[i].fd == sockfd) {

					struct homework_prot packet;
					memset(&packet, 0, sizeof(packet));
					recv_all(sockfd, (void *) &packet, sizeof(packet));
						
					if (strncmp(packet.command, "exit", 4) == 0 ||
						strncmp(packet.command, "refuse", 6) == 0) {

						client_running = false;
						
					} else if (strncmp(packet.command, "news", 4) == 0) {
						cout << packet.topic << " - ";
						print_udp(sockfd, packet.udp_message);

					}
					
				}
			}
		} 
	}

	close(sockfd);

	return 0;
}