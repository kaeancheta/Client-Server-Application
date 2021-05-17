#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <sys/types.h>
#include <netdb.h>

void main(int argc, char *argv[]){
    int address_fd, server_fd, valread; 
    struct sockaddr_in address; 
    int opt = 1; 
    int addrlen = sizeof(address); 
    char buffer[1024] = {0}; 

    struct addrinfo hints;
    struct addrinfo *result, *rp;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

    if ((address_fd = getaddrinfo(argv[1], argv[2], &hints, &result)) == -1){
        perror("getaddrinfo failed");
        exit(EXIT_FAILURE);
    }

    rp = result;
    while (rp != NULL){
        server_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (server_fd == -1) continue;
        if (connect(server_fd, rp->ai_addr, rp->ai_addrlen) != -1) break;
        rp = rp->ai_next;
    }

    freeaddrinfo(result);

    if (rp == NULL) {
        fprintf(stderr, "Could not connect\n");
        exit(EXIT_FAILURE);
    }

    char command[80];
    char delim[] = " \n";
    char *p;
    
    while (strcmp(command, "quit\n") != 0){
        printf("> ");
        fgets(command, 80, stdin);
        memset(buffer, 0, 1024);
        if (strncmp(command, "openRead", 8) == 0) {
            send(server_fd, command, strlen(command), 0);
            valread = read( server_fd , buffer, 1024);
            if (buffer[0] != ' ')
                printf("%s", buffer);
        }
        else if (strncmp(command, "openAppend", 10) == 0) {
            send(server_fd, command, strlen(command), 0);
            valread = read( server_fd , buffer, 1024);
            if (buffer[0] != ' ')
                printf("%s", buffer);
        }
        else if (strncmp(command, "read", 4) == 0) {
            send(server_fd, command, strlen(command), 0);
            valread = read( server_fd , buffer, 1024);
            if (buffer[0] != ' ')
                printf("%s\n", buffer);
        }
        else if (strncmp(command, "append", 6) == 0) {
            send(server_fd, command, strlen(command), 0);
            valread = read( server_fd , buffer, 1024);
            if (buffer[0] != ' ')
                printf("%s\n", buffer);
        }
        else if (strncmp(command, "close", 5) == 0) {
            send(server_fd, command, strlen(command), 0);
        }
    }
}