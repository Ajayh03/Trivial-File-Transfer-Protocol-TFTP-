#include "tftp.h"
#include "tftp_client.h"
#include <stdio.h>       
#include <stdlib.h>       
#include <string.h>      
#include <unistd.h>       
#include <fcntl.h>        
#include <sys/socket.h>  
#include <arpa/inet.h>    
#include<errno.h>         


int main() 
{
    tftp_client_t client;                              // Client connection structure
    tftp_packet packet;                                // Packet structure for data transfer
    memset(&client, 0, sizeof(client));               // Initialize client structure to zero
    packet.body.request.mode_flag=0;                  // Set default mode flag
    int option;                                        // User menu selection
    
    // Main loop - displays menu and processes user commands
    while (1) 
    {
        printf("Enter the option\n");
        printf("1. Connect\n2. Put\n3. Get\n4. Mode\n5. Exit\n");
        scanf("%d",&option);                           // Read user choice
        
        // Process user command
        switch(option)
        {
           // Case 1: Connect to TFTP server
           case 1:
           {
                connect_to_server(&client);
                break;
           }
           
           // Case 2: Upload file to server (Write Request)
           case 2:
           {
                put_file(&client,&packet);
                break;
           }
           
           // Case 3: Download file from server (Read Request)
           case 3:
           {
                get_file(&client,&packet);
                break;
           }
           
           // Case 4: Set transfer mode
           case 4:
           {
                int choice;
                printf("1. Default\n2. Octet\n3. Net Ascii\n");
                scanf("%d",&choice);
                
                // Set mode flag based on user selection
                if(choice == 1)
                {
                    packet.body.request.mode_flag = 0;   // Default mode
                }
                else if(choice == 2)
                {
                    packet.body.request.mode_flag = 2;   // Octet (binary) mode
                }
                else if(choice == 3)
                {
                    packet.body.request.mode_flag = 3;   // Net ASCII mode
                }
                break;
           }
           // Case 5: Disconnect and exit
           case 5:
           {    
                disconnect(&client);
                exit(0);
                break;
           }
           
           // Handle invalid input
           default:
           {
                printf("Invalid option\n"); 
           }
        }
    }
    return 0;
}

// Connect to TFTP server - initializes socket and validates server information
// Parameters: client - pointer to client structure
// Note: This function only sets up the connection; no packets are sent to the server yet
void connect_to_server(tftp_client_t *client) 
{
    // Create a UDP socket (SOCK_DGRAM) for communication
    client->sockfd = socket(AF_INET,SOCK_DGRAM,0);
    
    // Prompt and read server IP address from user
    printf("Enter the ip address :");
    getchar();                                         // Clear input buffer
    scanf(" %[^\n]",client->server_ip);              // Read IP address
    
    // Validate the IP address format

    // Loop through each character in IP address to validate format
    for(int i=0;i<strlen(client->server_ip);)
    {
        // Check if character is a digit (48-57 in ASCII) or a dot
        if((client->server_ip[i] >=48 && client->server_ip[i] <=57) || client->server_ip[i] == '.') 
        {
            // If character is a dot, verify it's followed by a digit
            if(client->server_ip[i] == '.')
            {
                int flag=0;
                // Check if next character is a digit
                if(client->server_ip[i+1] >=48 && client->server_ip[i+1] <=57)
                {
                    flag=1;
                }
                // If dot is not followed by digit, IP is invalid
                if(flag == 0)
                {
                    printf("Invalid IP\n");
                    return;
                }
            }
            i++;
        }
        // Reject any non-digit, non-dot characters
        else
        {
            printf("Invalid IP\n");
            return;
        }
    }
    // Prompt and read port number from user
    printf("Enter the port number :");
    scanf(" %d",&client->port);
    
    // Validate port number is in valid user port range (1024-49151)
    int flag=0;
    if(client->port >=1024 && client->port <= 49151)
    {
        flag = 1;
    }
    if(flag == 0)
    {
        printf("Invalid Port number\n");
        return;
    }

    // Configure server address structure for socket communication
    client->server_addr.sin_family = AF_INET;         // IPv4 address family
    client->server_addr.sin_port = htons(client->port); // Convert port to network byte order
    client->server_addr.sin_addr.s_addr = inet_addr(client->server_ip); // Convert IP to network format

}

// Upload file to TFTP server
// Parameters: client - pointer to client structure, packet - pointer to packet structure
void put_file(tftp_client_t *client,tftp_packet *packet) 
{
    // Prompt user for filename to upload
    char temp_file[100];
    printf("Enter the filename :");
    getchar();                                         // Clear input buffer
    scanf("%[^\n]",temp_file);                        // Read filename
    strcpy(packet->body.request.filename,temp_file);  // Copy filename to packet

    // Attempt to open file for reading (validate file exists)
    int fd = open(packet->body.request.filename,O_RDONLY);
    if(fd == -1)
    {
        printf("File not exist\n");
        return;
    }
    else
    {        
        packet->opcode = WRQ;                          // Set opcode to Write Request                          // Set opcode to Write Request
        send_request(client,packet);                   // Send initial WRQ opcode
        
        // Send transfer mode flag to server
        sendto(client->sockfd,&packet->body.request.mode_flag,4,0,(struct sockaddr *)&client->server_addr,sizeof(client->server_addr));
        
        // Send filename to server
        sendto(client->sockfd,packet->body.request.filename,strlen(packet->body.request.filename)+1,0,(struct sockaddr *)&client->server_addr,sizeof(client->server_addr));
        
        // Wait for server acknowledgement
        char ack[50];
        recvfrom(client->sockfd,ack,50,0,(struct sockaddr*)&client->server_addr,&client->server_len);
        // Check if server accepted the file transfer request
        if(strcmp(ack,"SUCCESS") != 0)
        {
            printf("Error in sending the file and mode\n");
            return;
        }
        else if(strcmp(ack,"SUCCESS") == 0)
        {
            int n;                                     // Bytes read from file
            int pack_no = 1;                           // Packet counter
            
            // Handle Default and NetASCII modes (modes 0 and 3)
            if(packet->body.request.mode_flag == 0 || packet->body.request.mode_flag == 3)
            {
                // Read file in chunks and send to server
                while((n =(read(fd,packet->body.data_packet.data,BUFFER_SIZE))) >= 0)
                {
                    // If EOF reached (0 bytes read), notify server and exit loop
                    if(n == 0)
                    {
                        char buf[100] = "sending_over";
                        sendto(client->sockfd,&buf,strlen(buf)+1,0,(struct sockaddr *)&client->server_addr,sizeof(client->server_addr));
                        break;
                    }

                    // Set data size and send file data
                    packet->body.data_packet.data_size = n;
                    send_file(client,packet);                   // Send data packet
                    
                    // Receive server acknowledgement of data size
                    int temp_data_size;
                    recvfrom(client->sockfd,&temp_data_size,4,0,(struct sockaddr*)&client->server_addr,&client->server_len);

                    // Resend if data size mismatch (possible corruption)
                    if(n != temp_data_size)
                    {
                        send_file(client,packet);                 
                    }
                    
                    // Clear data buffer for next read
                    memset(packet->body.data_packet.data, 0, sizeof(packet->body.data_packet.data));  
                }
            }
            // Handle Octet (binary) mode - send one byte at a time
            else if(packet->body.request.mode_flag == 2)
            {
                int data_ack = DATA;                   // Data opcode
                // Read and send file byte by byte
                while((n =(read(fd,&packet->body.data_packet.ch,1))) > 0)
                {
                    sendto(client->sockfd,&data_ack,4,0,(struct sockaddr *)&client->server_addr,sizeof(client->server_addr)); // Send DATA opcode
                    sendto(client->sockfd,&packet->body.data_packet.ch,1,0,(struct sockaddr *)&client->server_addr,sizeof(client->server_addr)); // Send byte
                }
                // Send final ACK to indicate end of file
                data_ack = ACK;
                sendto(client->sockfd,&data_ack,4,0,(struct sockaddr *)&client->server_addr,sizeof(client->server_addr));
                return;
            }
        }
    }
}

// Download file from TFTP server
// Parameters: client - pointer to client structure, packet - pointer to packet structure
void get_file(tftp_client_t *client,tftp_packet *packet) 
{
    // Send Read Request to server
    packet->opcode = RRQ;
    send_request(client,packet);                       // Send RRQ opcode
    
    // Send transfer mode flag to server
    sendto(client->sockfd,&packet->body.request.mode_flag,4,0,(struct sockaddr *)&client->server_addr,sizeof(client->server_addr));
    
    // Prompt user for filename to download
    char temp_file_buff[50];
    printf("Enter the file name :");
    getchar();                                         // Clear input buffer
    scanf("%[^\n]",temp_file_buff);                   // Read filename
    strcpy(packet->body.request.filename,temp_file_buff); // Copy to packet
    
    // Send filename to server
    sendto(client->sockfd,packet->body.request.filename,strlen(packet->body.request.filename)+1,0,(struct sockaddr *)&client->server_addr,sizeof(client->server_addr));
    int fd;                                            // File descriptor
    
    // Wait for server acknowledgement
    char ack[50];// Acknowledgement buffer
    recvfrom(client->sockfd,ack,50,0,(struct sockaddr*)&client->server_addr,&client->server_len);// Receive ack from server
    if(strcmp(ack,"SUCCESS") == 0)// If server indicates file is available
    {
        fd = open(packet->body.request.filename,O_TRUNC | O_WRONLY);// Try to open local file for writing (truncate if exists)
        if(fd == -1)// If file doesn't exist, create it
        {
            if(errno == EACCES)// If permission denied
            {
                printf("permission for write denied\n");
                return;
            }
            else if(errno == ENOENT)// If file not found, create new file
            {
                fd = open(packet->body.request.filename,O_CREAT | O_WRONLY,0666);// Create new file with read/write permissions
            }
        }
    }
    else if(strcmp(ack,"FAILURE") == 0)// If server indicates file not found
    {
        printf("File Not available in the server\n");
        return;
    }
    // Send acknowledgement to server about local file creation
    char ack_file[50];
    if(fd != -1)
    {
        // File successfully created/opened
        strcpy(ack_file,"SUCCESS");
        sendto(client->sockfd,ack_file,strlen(ack_file)+1,0,(struct sockaddr *)&client->server_addr,sizeof(client->server_addr));    
    }
    else if(fd == -1)
    {
        // Failed to create/open local file
        strcpy(ack_file,"FAILURE");
        sendto(client->sockfd,ack_file,strlen(ack_file)+1,0,(struct sockaddr *)&client->server_addr,sizeof(client->server_addr));
    }
    int n;                                            // Bytes received
    
    // Handle Default mode file reception
    if(packet->body.request.mode_flag == 0)
    {
        // Receive data until EOF
        while(1)
        {
            // Receive data size from server
            recvfrom(client->sockfd,&packet->body.data_packet.data_size, 4, 0, (struct sockaddr*)&client->server_addr,&client->server_len);
            fflush(stdout);
            
            // Receive actual data
            n = recvfrom(client->sockfd,packet->body.data_packet.data,packet->body.data_packet.data_size , 0, (struct sockaddr*)&client->server_addr,&client->server_len);
            
            // Check for EOF
            if(n == 0)
            {
                break;
            }
            
            // Write received data to local file
            write(fd,packet->body.data_packet.data,packet->body.data_packet.data_size);
            
            // Clear data buffer
            memset(packet->body.data_packet.data,0,sizeof(packet->body.data_packet.data));
            
            // Send acknowledgement to server
            sendto(client->sockfd,&n,4,0,(struct sockaddr *)&client->server_addr,sizeof(client->server_addr));
        }
        return;
    }
    // Handle Octet (binary) mode file reception - receive one byte at a time
    else if(packet->body.request.mode_flag == 2)
    {
        int data_ack;                                  // Opcode field
        while(1)
        {
            // Receive opcode from server
            recvfrom(client->sockfd,&data_ack,4,0,(struct sockaddr*)&client->server_addr,&client->server_len);
            
            // If opcode is DATA, receive one byte
            if(data_ack == DATA)
            {
                recvfrom(client->sockfd,&packet->body.data_packet.ch,1,0,(struct sockaddr*)&client->server_addr,&client->server_len);
                write(fd,&packet->body.data_packet.ch,1); // Write byte to file
            }
            // If opcode is ACK, file transfer complete
            else if(data_ack == ACK)
            {
                break;
            }
        }
        return;              
    }
    
    // Handle NetASCII mode file reception - removes CR characters
    if(packet->body.request.mode_flag == 3)
    {
        while(1)
        {
            // Receive data size from server
            recvfrom(client->sockfd,&packet->body.data_packet.data_size, 4, 0, (struct sockaddr*)&client->server_addr,&client->server_len);
            fflush(stdout);
            
            // Receive actual data
            n = recvfrom(client->sockfd,packet->body.data_packet.data,packet->body.data_packet.data_size , 0, (struct sockaddr*)&client->server_addr,&client->server_len);
            
            // Check for EOF
            if(n == 0)
            {
                break;
            }
            
            // Process data byte by byte, removing carriage returns (CR) for NetASCII mode
            for(int i=0;i<strlen(packet->body.data_packet.data);i++)
            {
                char ch;
                ch = packet->body.data_packet.data[i];
                // Write byte if not CR; NetASCII conversion
                if(ch != '\r')
                {
                    write(fd,&ch,1);
                }
                else
                {
                    continue;                          // Skip carriage returns
                }
            }
            
            // Clear data buffer
            memset(packet->body.data_packet.data,0,sizeof(packet->body.data_packet.data));
            
            // Send acknowledgement to server
            sendto(client->sockfd,&n,4,0,(struct sockaddr *)&client->server_addr,sizeof(client->server_addr));
        }
        return;
    }
}

// Close connection to TFTP server
// Parameters: client - pointer to client structure
void disconnect(tftp_client_t *client) 
{
    // Close socket file descriptor
    close(client->sockfd);
}

// Send initial request (RRQ or WRQ) opcode to server
// Parameters: client - pointer to client structure, packet - pointer to packet structure
void send_request(tftp_client_t *client,tftp_packet *packet)
{
    // For Write Request
    if(packet->opcode == WRQ)
    {
        packet->body.request.mode = WRQ;              // Set opcode to WRQ
        sendto(client->sockfd,&packet->body.request.mode,4,0,(struct sockaddr *)&client->server_addr,sizeof(client->server_addr)); // Send to server
    }
    // For Read Request
    else if(packet->opcode == RRQ)
    {
        packet->body.request.mode = RRQ;              // Set opcode to RRQ
        sendto(client->sockfd,&packet->body.request.mode,4,0,(struct sockaddr *)&client->server_addr,sizeof(client->server_addr)); // Send to server
    }
}
