#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <signal.h>

#define MAX_PORT_RANGE 65535  
#define MAX_BUFFER_SIZE 516   

// Timeout duration in seconds
#define DATA_TIMEOUT 1
#define CONNECTION_TIMEOUT 10

// Function to handle RRQ (Read Request) (will call send_data?)
void handle_rrq(int sockfd, struct sockaddr_in client_address, char *filename) {
    // Implement RRQ (Read Request) handling here
    
    // Open file
    FILE * read_file = fopen(filename, 'r');

    // Send error for failed open file
    if(read_file == -1){
        send_error(sockfd, &client_address, 1);
    }

    // Start read
    int block_num = 1;
    int file_size;
    fseek(read_file, 0, SEEK_END);
    file_size = ftell(read_file);
    fseek(read_file, 0, SEEK_SET);
    char * data;
    if(file_size < 512){
        data = calloc(file_size + 1, sizeof(char));
        fread(data, sizeof(char), file_size, read_file);
        data[file_size] = '\0';
        send_data(sockfd, &client_address, block_num, data, file_size+1);

        // Wait for ACK
        while(1){
            // Set up timer for data timeout
            struct timeval data_timeout;
            data_timeout.tv_sec = DATA_TIMEOUT;
            data_timeout.tv_usec = 0;

            // Set up timer for connection timeout
            struct timeval connection_timeout;
            connection_timeout.tv_sec = CONNECTION_TIMEOUT;
            connection_timeout.tv_usec = 0;

            // Initialize and set up file descriptors for select()
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(sockfd, &read_fds);

            // 10 second timer
            int select_result = select(sockfd + 1, &read_fds, NULL, NULL, &connection_timeout);

            int resend_result = select(sockfd + 1, &read_fds, NULL, NULL, &data_timeout);

            if (resend_result < 0) {
                perror("Error in select");
                exit(1);
            } else if (resend_result == 0) {
                // resend data
                send_data(sockfd, &client_address, block_num, data, file_size+1);
                continue;
            }

            if (select_result < 0) {
                perror("Error in select");
                exit(1);
            } else if (select_result == 0) {
                // Connection timeout
                printf("Connection timeout, aborting\n");
                exit(1);
            }
        }
    }
    else{ //require multiple reads
        while(){
            block_num++;
        }

    }

}

// Function to handle WRQ (Write Request) (will call handle data)
void handle_wrq(int sockfd, struct sockaddr_in client_address, char *filename) {
    // Implement WRQ (Write Request) handling here
}

// Function to handle DATA packet
void handle_data(){

}

// Function to handle ERROR packet
void handle_error(){

}

// Function to send DATA packet
void send_data(int sockfd, struct sockaddr_in * client_address, int block_num, char * data, int data_len){
    // Build packet
    char *packet = calloc(4+data_len, sizeof(char));
    *packet = htons(3);
    packet +=2;
    *packet = htons(block_num);
    strcat(packet, data);

    // Send packet
    sendto(sockfd, packet, 4+data_len, 0, (struct sockaddr *) client_address, sizeof(client_address));

    // Free packet
    free(packet);
}

// Function to send ERROR
void send_error(int sockfd, struct sockaddr_in * client_address, int error_code){
    char * packet; 
    char * msg;
    int packet_size = 5; // opcodes and end byte
    int msg_size;
    if(error_code == 0){ // Not defined (11), see error message
        packet_size += 11;
        msg_size = 11;
        msg = calloc(msg_size, sizeof(char));
        strcat(msg, "Not defined");
    }
    if(error_code == 1){ // File not found (14)
        packet_size += 14;
        msg_size = 14;
        msg = calloc(msg_size, sizeof(char));
        strcat(msg, "File not found");
    }
    if(error_code == 2){ // Access violation (16)
        packet_size += 16;
        msg_size = 16;
        msg_size = 11;
        msg = calloc(msg_size, sizeof(char));
        strcat(msg, "Access violation");        
    }
    if(error_code == 3){ // Disk full or allocation exceeded (33)
        packet_size += 33;
        msg_size = 33;
        msg_size = 11;
        msg = calloc(msg_size, sizeof(char));
        strcat(msg, "Disk full or allocation exceeded");        
    }
    if(error_code == 4){ // Illegal TFTP operation (22)
        packet_size += 22;
        msg_size = 22;
        msg_size = 11;
        msg = calloc(msg_size, sizeof(char));
        strcat(msg, "Illegal TFTP operation");
    }
    if(error_code == 5){ // Unknown transfer ID (19)
        packet_size += 19;
        msg_size = 19;
        msg_size = 11;
        msg = calloc(msg_size, sizeof(char));
        strcat(msg, "Unknown transfer ID");
    }
    if(error_code == 6){ // File already exists (19)
        packet_size += 19;
        msg_size = 19;
        msg_size = 11;
        msg = calloc(msg_size, sizeof(char));
        strcat(msg, "File already exists");
    }
    if(error_code == 7){ // No such user (12)
        packet_size += 12;
        msg_size = 12;
        msg_size = 11;
        msg = calloc(msg_size, sizeof(char));
        strcat(msg, "No such user");
    }
    // Create packet
    *packet = htons(5);
    packet += 2;
    *packet = htons(error_code);
    strcat(packet, msg);
    packet[packet_size] = '\0';

    // Send packet
    sendto(sockfd, packet, packet_size, 0, (struct sockaddr *) client_address, sizeof(client_address));

    // Free packet and msg
    free(packet);
    free(msg);

    // Terminate connection (should just exit child)
    exit(1);
}

// Function to send ACK
void send_ack(int sockfd, struct sockaddr_in * client_address, int block_num){
    // Build ACK packet
    char * packet = calloc(4, sizeof(char));
    *packet = htons(4);
    packet +=2;
    *packet =  htons(block_num);

    // Send packet
    sendto(sockfd, packet, 4, 0, (struct sockaddr *) client_address, sizeof(client_address));

    // Free packet
    free(packet);
}



int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s [start of port range] [end of port range]\n", argv[0]);
        exit(1);
    }

    int start_port = atoi(argv[1]);
    int end_port = atoi(argv[2]);

    if (start_port <= 0 || end_port <= 0 || start_port > MAX_PORT_RANGE || end_port > MAX_PORT_RANGE || start_port >= end_port) {
        fprintf(stderr, "Invalid port range\n");
        exit(1);
    }

    int sockfd;
    struct sockaddr_in server_address, client_address;
    socklen_t client_len = sizeof(client_address);
    char buffer[MAX_BUFFER_SIZE];


    // Create socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Error opening socket");
        exit(1);
    }

    // Initialize server_address structure
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;

    int current_port = start_port;
    int rc = getpid();
    int tid = start_port;
    while (1) {
        // Set the port for the server_address
        server_address.sin_port = htons(current_port);

        // Bind socket
        if (bind(sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
            perror("Error binding");
            current_port++;

            if (current_port > end_port) {
                fprintf(stderr, "No available ports in the specified range\n");
                exit(1);
            }

            continue;
        }

        // Set up timer for data timeout
        struct timeval data_timeout;
        data_timeout.tv_sec = DATA_TIMEOUT;
        data_timeout.tv_usec = 0;

        // Set up timer for connection timeout
        struct timeval connection_timeout;
        connection_timeout.tv_sec = CONNECTION_TIMEOUT;
        connection_timeout.tv_usec = 0;

        // Initialize and set up file descriptors for select()
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);

        int select_result = select(sockfd + 1, &read_fds, NULL, NULL, &connection_timeout);

        if (select_result < 0) {
            perror("Error in select");
            continue;
        } else if (select_result == 0) {
            // Connection timeout
            printf("Connection timeout, aborting\n");
            break;
        }

        if (FD_ISSET(sockfd, &read_fds)) {
            // Receive request
            memset(buffer, 0, sizeof(buffer));
            ssize_t bytes_received = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_address, &client_len);

            if (bytes_received < 0) {
                perror("Error receiving request");
                continue;
            }

            // Get next server TID (only for new connection)
            if(rc != 0){
                if(start_port + 1 < end_port){
                    tid = start_port + 1;

                    // Fork process
                    rc = fork;
                }
                else{// cancel connection?

                }
            }

            if(rc == 0){
                // Open new socket for connection
                // Set the port for the server_address

                server_address.sin_port = htons(tid);

                // Bind socket
                if (bind(sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
                    perror("Error binding");
                    tid++;

                    if (tid > end_port) {
                        fprintf(stderr, "No available ports in the specified range\n");
                        exit(1);
                    }

                    continue;
                }

                // Extract opcode
                unsigned short opcode;
                memcpy(&opcode, buffer, sizeof(unsigned short));
                opcode = ntohs(opcode);

                if (opcode == 1) {
                    // RRQ - Read Request
                    char *filename = buffer + 2;
                    handle_rrq(sockfd, client_address, filename);
                } else if (opcode == 2) {
                    // WRQ - Write Request
                    char *filename = buffer + 2;
                    handle_wrq(sockfd, client_address, filename);
                } else {
                    // Unsupported opcode
                    printf("Unsupported opcode: %d\n", opcode);
                    // Send ERROR packet
                    send_error(sockfd, &client_address, 4);
                }
            }
        }
    }

    close(sockfd);
    return 0;
}
