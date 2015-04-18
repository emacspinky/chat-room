//
//  server.cpp
//  server
//
//  Created by Mike Fanger on 4/16/15.
//  Copyright (c) 2015 Mike Fanger. All rights reserved.
//


// NEED TO HANDLE ARRAY and CLIENTCOUNT WHEN CLIENT LEAVES


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>  /* define socket */
#include <netinet/in.h>  /* define internet socket */
#include <netdb.h>       /* define internet socket */
#include <pthread.h>     /* POSIX threads */
#include <string.h>
#include <stdbool.h>

#define SERVER_PORT 9999
#define MAX_CLIENTS 10
#define MAX_MESSAGE 512

int clients[MAX_CLIENTS];
unsigned int client_count = 0;
pthread_mutex_t count_lock;
pthread_mutex_t array_lock;

void* clientHandler(void* arg);
int findEmptySlot(int clients[MAX_CLIENTS]);
void emitMessage(char message[MAX_MESSAGE], size_t bytes, int sender_sock, int clients[MAX_CLIENTS]);

int main(int argc, const char * argv[]) {
    
    // Prevent stdout from buffering
    setbuf(stdout, NULL);
    
    int srv_sock, clt_sock;
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = SERVER_PORT;
    
    struct sockaddr_in client_addr = { AF_INET };
    unsigned int client_len = sizeof(client_addr);
    
    //Client clients[MAX_CLIENTS];
    
    /* Initialize clients array */
//    for(int i = 0; i < MAX_CLIENTS; i++)
//    {
//        Client client;
//        client.empty = true;
//        clients[i] = client;
//    }
    
    /* Initialize clients array */
    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        clients[i] = -1;
    }
    
    /* Create server socket */
    if( (srv_sock = socket(AF_INET, SOCK_STREAM, 0) ) == -1 )
    {
        perror("SERVER: socket failed\n");
        exit(1);
    }
    
    /* Bind the socket to an internet port */
    if( bind(srv_sock, (struct sockaddr*) &server_addr, sizeof(server_addr)) == -1 )
    {
        perror("SERVER: bind failed\n");
        exit(1);
    }
    
    /* Listen for clients */
    if( listen(srv_sock, 10) == -1 )
    {
        perror("SERVER: listen failed\n");
        exit(1);
    }
    
    printf("SERVER is listening for clients to establish a connection\n");
    
    /* Wait for client connection request */
    while(true)
    {
        
        if( (clt_sock = accept(srv_sock, (struct sockaddr*)&client_addr, &client_len)) == -1 )
        {
            perror("server: accept failed\n");
            exit(1);
        }
        
        pthread_mutex_lock(&count_lock);
        if(client_count >= MAX_CLIENTS)
        {
            pthread_mutex_unlock(&count_lock);
            
            char buf[] = "Server is at maximum capacity, please try again later.";
            write(clt_sock, buf, sizeof(buf));
            close(clt_sock);
            continue;
        }
        
        client_count++;
        pthread_mutex_unlock(&count_lock);
        
        pthread_t tid;
        pthread_create(&tid, NULL, clientHandler, &clt_sock);
    }
    
    return 0;
}



void* clientHandler(void* arg)
{
    int clt_sock = *(int*)arg;
    char buf[512]; bzero(buf, sizeof(buf));
    long bytes_read;
    char client_name[512];
    
    /* Insert new client into clients array */
    int idx = findEmptySlot(clients);
    pthread_mutex_lock(&array_lock);
    clients[idx] = clt_sock;
    pthread_mutex_unlock(&array_lock);
    
    
    
    
    // Get client name
//    while(!done && (bytes_read = read(clt_sock, buf, sizeof(buf))) != 0)
//    {
//        strcpy(client_name, buf);
//        strcat(buf, " connected!\n");
//        
//        write(clt_sock, buf, bytes_read);
//        bzero(buf, sizeof(buf));
//        
//        done = true;
//    }
    
    /* Read client name */
    bytes_read = read(clt_sock, buf, sizeof(buf));
    char greeting[] = " connected to the server";
    strcpy(client_name, buf);
    strcat(buf, greeting);
    printf("%s\n", buf);
    
    /* Inform all clients of the connected user */
    pthread_mutex_lock(&array_lock);
    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(clients[i] == -1) continue;
        
        write(clients[i], buf, bytes_read + sizeof(greeting) - 1); // NOT SURE IF -1 NEEDED
    }
    pthread_mutex_unlock(&array_lock);
    
    bzero(buf, sizeof(buf));
    
    /* Read messages from the client and emit to all other connect clients */
    while( (bytes_read = read(clt_sock, buf, sizeof(buf))) != 0)
    {
        printf("%s: %s\n", client_name, buf);
        
        char message[MAX_MESSAGE]; bzero(message, sizeof(message));
        strcpy(message, client_name);
        strcat(message, ": ");
        strcat(message, buf);
        
        emitMessage(message, strlen(message), clt_sock, clients);
        bzero(buf, sizeof(buf));
    }
    
    printf("Leaving client handler thread\n");
    
    return NULL;
}

// Returns index of an empty slot in the clients array.
// If array is full, returns -1
// THREAD SAFE
int findEmptySlot(int clients[MAX_CLIENTS])
{
    int idx = -1;
    bool done = false;
    
    for(int i = 0; i < MAX_CLIENTS && !done; i++)
    {
        pthread_mutex_lock(&array_lock);
        if(clients[i] == -1)
        {
            idx = i;
            done = true;
        }
        pthread_mutex_unlock(&array_lock);
    }
    
    return idx;
}

// Sends message to all connect clients, except the client that the message
// originated from.
// THREAD SAFE
void emitMessage(char message[MAX_MESSAGE], size_t bytes, int sender_sock, int clients[MAX_CLIENTS])
{
    pthread_mutex_lock(&array_lock);
    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(clients[i] == -1) continue;
        if(clients[i] == sender_sock) continue;
        
        write(clients[i], message, bytes);
    }
    pthread_mutex_unlock(&array_lock);
}

