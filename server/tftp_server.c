#include "tftp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include<errno.h>

int main() {
    int sockfd; // Socket file descriptor
    struct sockaddr_in server_addr, client_addr; // Server and client addresses
    socklen_t client_len ; // Length of client address
    tftp_packet packet; // TFTP packet structure
    // Create UDP socket
    sockfd = socket(AF_INET,SOCK_DGRAM,0); // UDP socket
    server_addr.sin_family = AF_INET; // IPv4
    server_addr.sin_port = htons(PORT); // Port number
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Localhost

    bind(sockfd,(struct sockaddr *)&server_addr,sizeof(server_addr)); // Bind socket to address
    // Set socket timeout option
    skip:
    printf("Waiting for a client to connect.....\n");
    
    recvfrom(sockfd,&packet.body.request.mode,4,0,(struct sockaddr *)&client_addr,&client_len);// Receive mode (RRQ or WRQ)
    printf("mode-> %d\n",packet.body.request.mode);// Print received mode
    fflush(stdout);
    if(packet.body.request.mode == WRQ)// If mode is WRQ (Write Request)
    {
            recvfrom(sockfd,&packet.body.request.mode_flag,4,0,(struct sockaddr *)&client_addr,&client_len);// Receive mode flag
            printf("mode flag > %d\n",packet.body.request.mode_flag);// Print received mode flag
            fflush(stdout);
            recvfrom(sockfd,packet.body.request.filename,sizeof(packet.body.request.filename),0,(struct sockaddr *)&client_addr,&client_len); // Receive filename 
            printf("file_name-> %s\n",packet.body.request.filename);// Print received filename
            int fd = open(packet.body.request.filename,O_CREAT|O_EXCL|O_WRONLY,0666);// Open file for writing
            if(fd == -1)// If file already exists
            {
                if(errno == EEXIST)
                {
                    fd = open(packet.body.request.filename,O_TRUNC|O_WRONLY,0666);
                }
            }

            //acknowlegment
            char ack[50]="SUCCESS";// Acknowledgment message
            sendto(sockfd,ack,strlen(ack)+1,0,(struct sockaddr*)&client_addr,sizeof(client_addr));// Send acknowledgment to client
            printf("TFTP Server listening on port %d...\n", PORT);// Print server listening message
            fflush(stdout);// Flush output buffer
            // Main loop to handle incoming requests
            int pack_no;// Packet number
            if(packet.body.request.mode_flag == 0)// If mode flag is 0 (binary mode)
            {
                while (1)// Loop until all data is received
                {
                    int n = recvfrom(sockfd,packet.body.data_packet.data,BUFFER_SIZE , 0, (struct sockaddr *)&client_addr, &client_len);// Receive data packet from client
                    if(strcmp(packet.body.data_packet.data,"sending_over") == 0)// Check if transmission is complete
                    {
                        break;// Exit loop when transmission ends
                    }
                    recvfrom(sockfd,&packet.body.data_packet.data_size, 4, 0, (struct sockaddr *)&client_addr, &client_len);// Receive data size
                    //printf("data size send from client: %d\n",packet.body.data_packet.data_size);
                    write(fd,packet.body.data_packet.data,packet.body.data_packet.data_size);// Write data to file
                    memset(packet.body.data_packet.data,0,sizeof(packet.body.data_packet.data));// Clear data buffer
                    sendto(sockfd,&n,4,0,(struct sockaddr*)&client_addr,sizeof(client_addr));// Send acknowledgment with packet size
                }
                goto skip;// Skip to next section
            }
            else if(packet.body.request.mode_flag == 2)// Else if mode flag is 2 (text mode)
            {
                int data_ack;// Acknowledgment variable
                while(1)// Loop until all data is received
                {
                    recvfrom(sockfd,&data_ack,4,0,(struct sockaddr *)&client_addr,&client_len);
                    if(data_ack == DATA)// Check if data request received
                    {
                        recvfrom(sockfd,&packet.body.data_packet.ch,1,0,(struct sockaddr *)&client_addr,&client_len);// Receive single character
                        write(fd,&packet.body.data_packet.ch,1);// Write character to file
                    }
                    else if(data_ack == ACK)// Check if acknowledgment received
                    {
                        break;// Exit loop
                    }
                }
                goto skip;// Skip to end
            }
            else if(packet.body.request.mode_flag == 3)// Else if mode flag is 3 (CRLF mode)
            {
                while (1)// Loop until transmission ends
                {
                    int n = recvfrom(sockfd,packet.body.data_packet.data,BUFFER_SIZE , 0, (struct sockaddr *)&client_addr, &client_len);// Receive data packet
                    if(strcmp(packet.body.data_packet.data,"sending_over") == 0)// Check if transmission is complete
                    {
                        break;// Exit loop
                    }
                    recvfrom(sockfd,&packet.body.data_packet.data_size, 4, 0, (struct sockaddr *)&client_addr, &client_len);// Receive data size
                    for(int i=0;i<strlen(packet.body.data_packet.data);i++)// Loop through each character
                    {
                        char ch = packet.body.data_packet.data[i];// Get current character
                        if(ch != '\n')// If not newline
                        {
                            write(fd,&ch,1);// Write character as is
                        }
                        else// If newline
                        {
                            ch = '\r';// Convert to carriage return
                            write(fd,&ch,1);// Write carriage return
                            ch = '\n';// Set to newline
                            write(fd,&ch,1);// Write newline
                        }
                    }
                        memset(packet.body.data_packet.data,0,sizeof(packet.body.data_packet.data));// Clear data buffer
                        sendto(sockfd,&n,4,0,(struct sockaddr*)&client_addr,sizeof(client_addr));// Send acknowledgment
                }
                goto skip;// Skip to end
            }
    }

    else if(packet.body.request.mode == RRQ)// If mode is Read Request
    {
        recvfrom(sockfd,&packet.body.request.mode_flag,4,0,(struct sockaddr *)&client_addr,&client_len);// Receive mode flag
        printf("mode flag > %d\n",packet.body.request.mode_flag);// Print mode flag
        fflush(stdout);// Flush output buffer

        recvfrom(sockfd,packet.body.request.filename,sizeof(packet.body.request.filename),0,(struct sockaddr *)&client_addr,&client_len);// Receive filename
        printf("file_name->%s\n",packet.body.request.filename);// Print filename
        int fd = open(packet.body.request.filename,O_RDONLY);// Open file for reading
        char ack[50];// Acknowledgment buffer
        if(fd != -1)// If file opened successfully
        {
            strcpy(ack,"SUCCESS");// Set success message
            sendto(sockfd,ack,strlen(ack)+1,0,(struct sockaddr*)&client_addr,sizeof(client_addr));// Send success acknowledgment
        }
        else// If file open failed
        {
            strcpy(ack,"FAILURE");// Set failure message
            sendto(sockfd,ack,strlen(ack)+1,0,(struct sockaddr*)&client_addr,sizeof(client_addr));// Send failure acknowledgment
            goto skip;// Skip to end
        }
        char ack_file[50];// File acknowledgment buffer
        recvfrom(sockfd,ack_file,sizeof(ack_file),0,(struct sockaddr *)&client_addr,&client_len);// Receive file acknowledgment

        if(strcmp(ack_file,"SUCCESS") == 0)// If client acknowledges success
        {
            int n;// Bytes read
            if(packet.body.request.mode_flag == 0 || packet.body.request.mode_flag == 3)// If binary or CRLF mode
            {
                while((n =(read(fd,packet.body.data_packet.data,BUFFER_SIZE))) >= 0)// Read data from file
                {
                    packet.body.data_packet.data_size = n;// Set data size
                    send_file(sockfd,client_addr,client_len,&packet);// Send data packet to client
                    if(n == 0)// If no more data
                    {
                        break;// Exit loop
                    }
                    int size;// Received size
                    recvfrom(sockfd,&size,4,0,(struct sockaddr *)&client_addr,&client_len);// Receive acknowledgment size

                    if(size != n)// If sizes don't match
                    {
                        send_file(sockfd,client_addr,client_len,&packet);// Resend packet
                    }
                    memset(packet.body.data_packet.data,0,sizeof(packet.body.data_packet.data));// Clear data buffer
                }
                goto skip;// Skip to end
            }
            else if(packet.body.request.mode_flag == 2)// Else if text mode
            {
                int data_ack = DATA;// Set data flag
                while((n =(read(fd,&packet.body.data_packet.ch,1))) > 0)// Read single character
                {
                    sendto(sockfd,&data_ack,4,0,(struct sockaddr*)&client_addr,sizeof(client_addr));// Send data flag
                    sendto(sockfd,&packet.body.data_packet.ch,1,0,(struct sockaddr*)&client_addr,sizeof(client_addr));// Send character
                }
                data_ack = ACK;// Set acknowledgment flag
                sendto(sockfd,&data_ack,4,0,(struct sockaddr*)&client_addr,sizeof(client_addr));// Send final acknowledgment
                goto skip;// Skip to end
            }
        }
        else if(strcmp(ack_file,"FAILURE") == 0)// If client sends failure acknowledgment
        {
            return 0;// Exit function
        }
    }
}





