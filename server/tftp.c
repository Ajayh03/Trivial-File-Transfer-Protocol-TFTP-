/* Common file for server & client */

#include "tftp.h"

void send_file(int sockfd, struct sockaddr_in client_addr, socklen_t client_len,tftp_packet *packet) 
{
    // Implement file sending logic here
    sendto(sockfd,&packet->body.data_packet.data_size,4,0,(struct sockaddr*)&client_addr,sizeof(client_addr));
    sendto(sockfd,packet->body.data_packet.data,packet->body.data_packet.data_size,0,(struct sockaddr*)&client_addr,sizeof(client_addr));
}
