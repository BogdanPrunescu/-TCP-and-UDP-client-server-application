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

#include "helpers.h"

#include <bits/stdc++.h>

int send_all(int sockfd, void *buff, size_t len) {

    size_t bytes_sent = 0;
    size_t bytes_remaining = len;
    char *buffer = (char *) buff;

    while (bytes_remaining) {
        bytes_sent += send(sockfd, buffer + bytes_sent, bytes_remaining, 0);
        bytes_remaining = len - bytes_sent;
    }

    return bytes_sent;

}

int recv_all(int sockfd, void *buff, size_t len) {
    size_t bytes_received = 0;
    size_t bytes_remaining = len;
    char *buffer = (char *) buff;

    while (bytes_remaining) {
        bytes_received += recv(sockfd, buffer + bytes_received, bytes_remaining, 0);
        bytes_remaining = len - bytes_received;
    }

    return bytes_received;
}