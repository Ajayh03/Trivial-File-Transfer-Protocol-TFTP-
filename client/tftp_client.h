#ifndef TFTP_CLIENT_H
#define TFTP_CLIENT_H

typedef struct {
    int sockfd;
    struct sockaddr_in server_addr;
    socklen_t server_len;
    char server_ip[INET_ADDRSTRLEN];//store ip address
    int port;//give a var for port no
} tftp_client_t;

// Function prototypes
void connect_to_server(tftp_client_t *client);
void put_file(tftp_client_t *client,tftp_packet *packet);
void get_file(tftp_client_t *client,tftp_packet *packet);
void disconnect(tftp_client_t *client);
void send_request(tftp_client_t *client,tftp_packet *packet);
void receive_request(int sockfd, struct sockaddr_in server_addr, char *filename, int opcode);
void send_file(tftp_client_t *client,tftp_packet *packet); 
#endif