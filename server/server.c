//
//  main.c
//  server
//
//  Created by Mike Fanger on 4/16/15.
//  Copyright (c) 2015 Mike Fanger. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>  /* define socket */
#include <netinet/in.h>  /* define internet socket */
#include <netdb.h>       /* define internet socket */

#define SERVER_PORT 9999

int main(int argc, const char * argv[]) {
    
    printf("Hello, World!\n");
    
    int sd, ns;
    long k;
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = SERVER_PORT;

    
    struct sockaddr_in client_addr = { AF_INET };
    unsigned int client_len = sizeof(client_addr);
    char buf[512], *host;
    
    
    /* Create socket */
    if( (sd = socket(AF_INET, SOCK_STREAM, 0) ) == -1 )
    {
        perror( "server: socket failed" );
        exit( 1 );
    }
    
    /* Bind the socket to an internet port */
    if( bind(sd, (struct sockaddr*) &server_addr, sizeof(server_addr)) == -1 )
    {
        perror( "server: bind failed" );
        exit( 1 );
    }
    
    /* listen for clients */
    if( listen(sd, 4) == -1 )
    {
        perror( "server: listen failed" );
        exit( 1 );
    }
    
    printf("SERVER is listening for clients to establish a connection\n");
    
    /* Wait for connection request */
    if( (ns = accept( sd, (struct sockaddr*)&client_addr, &client_len ) ) == -1 )
    {
        perror( "server: accept failed" );
        exit( 1 );
    }
    
    printf("Client connection accepted!\n");
    
    while( (k = read(ns, buf, sizeof(buf))) != 0)
    {    printf("SERVER RECEIVED: %s\n", buf);
        write(ns, buf, k);
    }
    
    
    return 0;
}
