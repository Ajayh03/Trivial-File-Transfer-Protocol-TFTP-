/* Common file for server & client*/

#ifndef TFTP_H
#define TFTP_H

#include <stdint.h>
#include <arpa/inet.h>

#define PORT 6969
#define BUFFER_SIZE 513  // TFTP data packet size (512 bytes data + 4 bytes header)
#define TIMEOUT_SEC 5    // Timeout in seconds

// TFTP OpCodes
typedef enum {
    RRQ = 1,  // Read Request
    WRQ = 2,  // Write Request
    DATA = 3, // Data Packet
    ACK = 4,  // Acknowledgment
    //ERROR = 5 // Error Packet
} tftp_opcode;

// TFTP Packet Structure
typedef struct {
    uint16_t opcode; // Operation code (RRQ/WRQ/DATA/ACK/ERROR)
    union {
        struct {
            char filename[256];
            int mode;  // Typically "octet"
            int mode_flag;
        } request;  // RRQ and WRQ
        struct {
            uint16_t block_number;
            char data[513];
            int data_size;
            char ch;
        } data_packet; // DATA
        struct {
            uint16_t block_number;
            int data_size;
        } ack_packet; // ACK
        struct {
            uint16_t error_code;
            char error_msg[512];
        } error_packet; // ERROR
    } body;
} tftp_packet;

#endif // TFTP_H
