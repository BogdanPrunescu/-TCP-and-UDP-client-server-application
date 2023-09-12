#ifndef _UDP_PACKET_H
#define _UDP_PACKET_H

#define MAX_MESSAGE_LENGTH 100

int send_all(int sockfd, void *buff, size_t len);
int recv_all(int sockfd, void *buff, size_t len);
// int recv_all_udp(int sockfd, void *buff, size_t len);


struct udp_packet {
    char topic[50];
    uint8_t tip_date;
    char continut[1500];
};

/**
 * The server and subscribers will communicate using the following fields
 * 
 * Commands that the client will send to the server:
 * subscribe -> tell the server that the client will want to subscribe to a
 * topic
 * unsubscribe -> unsubscribe from the topic
 * exit -> the client will close. The server will remove the client from poll
 * and from subscribers.
 * 
 * Commands that the server will send to the client:
 * exit -> The server will close. The client will close as well
 * news -> The server send a message from an UDP client. The client will print
 * the message. Inside the topic field the protocol will send the ip:port of
 * the udp client.
 * refuse -> The server refused the client's connect attempt. This will happen
 * if the id of the client is already present in the database.
 * 
 * 
 * Obs. Command field will be used to also send the client's id every time the
 * client will connect to the server
*/
struct homework_prot {
    char command[15];
    char topic[50];
    int sf;
    struct udp_packet udp_message;
};

#endif