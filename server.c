#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <sys/types.h>
#include <netdb.h>
#include <float.h>
#include <pthread.h>
#include <semaphore.h>

struct sharedFile {
    char* fileName;
    int state;
};

struct open {
    FILE *fd;
    char* fileName;
};

sem_t mutex;
char client_message[2000];
char buffer[1024];
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
struct sharedFile sharedFiles[100];
int sharedFileNum = 0;

void * sThread(void *arg){
    // Get socket descriptor and initialize var i counter
    int i;
    int sock = *(int*)arg;
    int valread;

    // initialize char pointer p for tokenizing and a delim and state var, found var for later use
    char *p;
    char delim[] = " \n";
    int state;
    int found;

    // initialize open and append to show that this client thread does not have any files open for read or append
    int open = 0;
    int append = 0;
    struct open reading;
    struct open appending; 

    // while loop continues until client types "quit\n"
    while (strcmp(buffer, "quit\n") != 0) {
        // parse commands
        
        if (strncmp(buffer, "openRead", 8) == 0) {
            // copy command for printing later if needed
            char copy[1024] = {0};
            strcpy(copy, buffer);

            // get the file name for opening
            p = strtok(buffer, delim);
            p = strtok(NULL, delim);
            char *filename = malloc(strlen(p));
            strcpy(filename, p);

            // set state to 0 and found to 0, will be used to see if file can be opened
            state = 0;
            found = 0;
            // semaphore here to make sure only one thread at a time is altering the sharedFiles array
            sem_wait(&mutex);

            // go through all sharedFiles to find if filename is in sharedFiles 
            // and if it is available for opening
            for (i = 0; i < sharedFileNum; i++){
                if (strcmp(sharedFiles[i].fileName, filename) == 0) {
                    // file has been found; set found to 1 and set state to sharedFiles[i].state
                    found = 1;
                    state = sharedFiles[i].state;
                }
            }
            
            // find out what to do to a file in each case
            if (open == 0 && state != 2) {
                // if the current client thread does not have any files open, and
                // the file is available for opening whether it was found in sharedFiles or not,
                // increment state to 1, set current thread's open file to state
                state = 1;
                open = 1;
                
                if (found == 0) {
                    // if the file is not in the sharedFiles array, add it to the array
                    struct sharedFile newFile;
                    newFile.fileName = malloc(strlen(filename));
                    strcpy(newFile.fileName, filename);
                    newFile.state = state;
                    sharedFiles[sharedFileNum] = newFile;
                    sharedFileNum++;
                }
                else {
                    // if the file is in the sharedFiles array, update its state
                    for (i = 0; i < sharedFileNum; i++){
                        if (strcmp(sharedFiles[i].fileName, filename) == 0) {
                            sharedFiles[i].state = state;
                        }
                    }
                }
                
                struct open newReading;
                newReading.fd = fopen(filename, "rb");
                newReading.fileName = malloc(strlen(filename));
                strcpy(newReading.fileName, filename);
                reading = newReading;

                send(sock, " ", 1, 0);
                printf("%s", copy);
            }
            else if (state == 2) {
                // the file is opened by another client
                char *error = "The file is open by another client\n";
                printf("%s", copy);
                printf("%s", error);
                send(sock, error, strlen(error), 0);
            }
            else {
                // this client thread already has a file opened, i.e. open != 0
                char *error = "A file is already open for reading\n";
                printf("%s", copy);
                printf("%s", error);
                send(sock, error, strlen(error), 0);
            }
            sem_post(&mutex);
        }
        if (strncmp(buffer, "openAppend", 10) == 0) {
            // copy command for printing later if needed
            char copy[1024] = {0};
            strcpy(copy, buffer);

            // get the file name for opening
            p = strtok(buffer, delim);
            p = strtok(NULL, delim);
            char *filename = malloc(strlen(p));
            strcpy(filename, p);

            // set state to 0 and found to 0, will be used to see if file can be opened
            state = 0;
            found = 0;
            // semaphore here to make sure only one thread at a time is altering the sharedFiles array
            sem_wait(&mutex);

            // go through all sharedFiles to find if filename is in sharedFiles 
            // and if it is available for opening
            for (i = 0; i < sharedFileNum; i++){
                if (strcmp(sharedFiles[i].fileName, filename) == 0) {
                    // file has been found; set found to 1 and set state to sharedFiles[i].state
                    found = 1;
                    state = sharedFiles[i].state;
                }
            }
            // find out what to do to a file in each case
            if (append == 0 && state == 0) {
                // if the current client thread does not have any files open for appending, and
                // the file is available for opening whether it was found in sharedFiles or not,
                // increment state to 1, set current thread's open file to state
                state = 2;
                append = 1;

                if (found == 0) {
                    // if the file is not in the sharedFiles array, add it to the array
                    struct sharedFile newFile;
                    newFile.fileName = malloc(strlen(filename));
                    strcpy(newFile.fileName, filename);
                    newFile.state = state;
                    sharedFiles[sharedFileNum] = newFile;
                    sharedFileNum++;
                }
                else {
                    // if the file is in the sharedFiles array, update its state
                    for (i = 0; i < sharedFileNum; i++){
                        if (strcmp(sharedFiles[i].fileName, filename) == 0) {
                            sharedFiles[i].state = state;
                        }
                    }
                }
                struct open newAppending;
                newAppending.fd = fopen(filename, "a");
                newAppending.fileName = malloc(strlen(filename));
                strcpy(newAppending.fileName, filename);
                appending = newAppending;

                send(sock, " ", 1, 0);
                printf("%s", copy);
            }
            else if (append != 0) {
                // this client thread already has a file opened, i.e. append != 0
                char *error = "A file is already open for appending\n";
                printf("%s", copy);
                printf("%s", error);
                send(sock, error, strlen(error), 0);
            }
            else if (state != 0) {
                // the file is opened by another client
                char *error = "The file is open by another client\n";
                printf("%s", copy);
                printf("%s", error);
                send(sock, error, strlen(error), 0);
            }
            sem_post(&mutex);
        }
        else if (strncmp(buffer, "read", 4) == 0) {
            // copy command for printing later if needed
            char copy[1024] = {0};
            strcpy(copy, buffer);
            if (open == 0) {
                char *error = "File not open";
                printf("%s", copy);
                printf("%s\n", error);
                send(sock, error, strlen(error), 0);
            }
            else {
                // get the number of bytes for reading
                p = strtok(buffer, delim);
                p = strtok(NULL, delim);
                int bytes = atoi(p);

                // read the bytes onto b
                unsigned char b[17];
                memset(b, 0, 17);
                int read = 0;
                read = fread(b, 1, bytes, reading.fd);
                // send the read bytes to client
                if (read == 0) send(sock, " ", 1, 0);
                else send(sock, b, strlen(b), 0);
                printf("%s", copy);
            }
        }
        else if (strncmp(buffer, "append", 6) == 0) {
            // copy command for printing later if needed
            char copy[1024] = {0};
            strcpy(copy, buffer);
            if (append == 0) {
                char *error = "File not open";
                printf("%s", copy);
                printf("%s\n", error);
                send(sock, error, strlen(error), 0);
            }
            else {
                // get the content for appending
                p = strtok(buffer, delim);
                p = strtok(NULL, delim);
                char *content = malloc(strlen(p));
                strcpy(content, p);
                printf("%s", copy);
                fprintf(appending.fd, "%s", content);
                send(sock, " ", 1, 0);
            }
        }
        else if (strncmp(buffer, "close", 5) == 0) {
            // copy command for printing later if needed
            char copy[1024] = {0};
            strcpy(copy, buffer);

            // get the flie name for closing
            p = strtok(buffer, delim);
            p = strtok(NULL, delim);
            char *filename = malloc(strlen(p));
            strcpy(filename, p);

            // close the file and send command;
            if (open != 0 && strcmp(reading.fileName, filename) == 0) {
                fclose(reading.fd);
                open = 0;
            }
            else if (append != 0 && strcmp(appending.fileName, filename) == 0) {
                fclose(appending.fd);
                append = 0;
            }
            sem_wait(&mutex);
            for (i = 0; i < sharedFileNum; i++){
                if (strcmp(sharedFiles[i].fileName, filename) == 0) {
                    sharedFiles[i].state = 0;
                }
            }
            sem_post(&mutex);

            printf("%s", copy);
            
        }
        memset(buffer, 0, 1024);
        
        valread = read( sock , buffer, 1024);
        if(valread == 0){
            fflush(stdout);
            break;
        }
        else if(valread == -1){
            perror("read failed");
        }
    }
    close(sock);
    pthread_exit(NULL);
}

void main(int argc, char *argv[]){
    sem_init(&mutex, 0, 1);
    int listen_fd, address_fd, server_fd, new_socket; 
    struct sockaddr_in address; 
    struct sockaddr_storage storage;
    socklen_t size;
    int opt = 1; 
    int addrlen = sizeof(address); 
    
    memset(buffer, 0, 1024);
    char hostname[1024];
    hostname[1023] = '\0';
    gethostname(hostname, 1023);

    struct addrinfo hints;
    struct addrinfo *result, *rp;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_ADDRCONFIG;
    hints.ai_protocol = 0;          
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    if ((address_fd = getaddrinfo(hostname, argv[argc - 1], &hints, &result)) == -1){
        perror("getaddrinfo failed");
        exit(EXIT_FAILURE);
    }

    rp = result;
    while (rp != NULL){
        server_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (server_fd == -1) continue;
        if (bind(server_fd, rp->ai_addr, rp->ai_addrlen) == 0) break;
        rp = rp->ai_next;
    }

    freeaddrinfo(result);

    if (rp == NULL) {
        fprintf(stderr, "Could not bind\n");
        exit(EXIT_FAILURE);
    }
    
    if ((listen_fd = listen(server_fd, 1024)) == -1) { 
        perror("listen"); 
        exit(EXIT_FAILURE); 
    }
    
    printf("server started\n");

    pthread_t tid[60];
    int i = 0;
	
    while( 1 ){
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) == -1) { 
            perror("accept"); 
            exit(EXIT_FAILURE); 
        }
        
        if( pthread_create( &tid[i++] , NULL ,  sThread , (void*) &new_socket) < 0){
            perror("could not create thread");
        }
        //Now join the thread , so that we dont terminate before the thread
        if( i >= 50){
          i = 0;
          while(i < 50){
            pthread_join(tid[i++],NULL);
          }
          i = 0;
        }
    }
    
}