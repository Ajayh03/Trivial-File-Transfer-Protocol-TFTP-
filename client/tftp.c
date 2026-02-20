/* Common file for server & client */

#include "tftp.h"
#include "tftp_client.h"

void send_file(tftp_client_t *client,tftp_packet *packet) 
{
    sendto(client->sockfd,packet->body.data_packet.data,packet->body.data_packet.data_size,0,(struct sockaddr *)&client->server_addr,sizeof(client->server_addr));
    sendto(client->sockfd,&packet->body.data_packet.data_size,4,0,(struct sockaddr *)&client->server_addr,sizeof(client->server_addr));
}
