/************************************************************************/ 
/*   PROGRAM NAME: client.c  (works with server.c)                      */
/*                                                                      */ 
/*   Client creates a socket to connect to Server.                      */ 
/*   When the communication established, Client chooses username and    */
/*   can then message the server. Messages can also be received from the*/
/*   server and displayed to the user.                                  */
/*                                                                      */
/*   To run this program, first compile and run the server.c file       */
/*   on a server machine. Then run the client program on another        */ 
/*   machine.                                                           */ 
/*                                                                      */ 
/*   COMPILE:    gcc -o client client.c -lpthread -lnsl                 */
/*   TO RUN:     client  server-hostname                                */
/*                                                                      */ 
/************************************************************************/ 

//
//  client.c
//  CS 3800 Assignment 3
//

#include <stdio.h> 
#include <sys/types.h>   /* for POSIX threads */
#include <sys/socket.h>  /* define socket */ 
#include <netinet/in.h>  /* define internet socket */ 
#include <netdb.h>       /* define internet socket */ 
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>
 
#define SERVER_PORT 9999     /* chosen server port number */

const int BUFFER_LENGTH = 512; // message buffer length

int socketNumber = 0; // file descriptor number assigned to socket upon connection
pthread_t threadID; // thread used for client read handler

/* --------------------------- FUNCTION PROTOTYPES --------------------------- */

void *readThread( void *ptr );
void exitHandler(int sig);

/* --------------------------- MAIN PROGRAM --------------------------- */

int main( int argc, char* argv[] ) 
{
    /* INITIALIZE SOCKET CONNECTION PARAMETERS */
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    char buf[BUFFER_LENGTH]; // message buffer
    struct hostent *hostInfo;
    bool bServerAccepted = false; // set to true when the server has accepted the client
    bool bClientExit = false; // set to true when the user exits the client

    signal(SIGINT,exitHandler); // SIGINTs will be handled by the exitHandler function

    /* VALIDATE INITIAL ARGUMENTS -- EXECUTABLE AND HOSTNAME */
    if( argc != 2 ) 
    {
        printf("CLIENT ERROR: Invalid number of arguments! Should be client executable and hostname.\n\n");
        exit( EXIT_FAILURE );
    }

    /* GET HOST INFORMATION */
    if( ( hostInfo = gethostbyname( argv[1] ) ) == NULL )
    {
        printf("CLIENT ERROR: %s is an unknown host!\n\n",argv[1]);
        exit( EXIT_FAILURE );
    }
    bcopy(hostInfo->h_addr_list[0], (char*)&server_addr.sin_addr, hostInfo->h_length);
 
    /* CREATE CLIENT SOCKET */
    if( ( socketNumber = socket( AF_INET, SOCK_STREAM, 0 ) ) == -1 )
    {
        perror( "CLIENT ERROR: SOCKET CREATION FAILED!\n\n" );
        exit( EXIT_FAILURE );
    } 

    /* CONNECT TO SERVER SOCKET */
    if( connect( socketNumber, (struct sockaddr*)&server_addr, sizeof(server_addr) ) == -1 )
    { 
        perror( "CLIENT ERROR: connect() FAILED!\n\n" );
        exit( EXIT_FAILURE );
    } 
 
    printf("CONNECTED TO CHATROOM!\n");
    printf("Input your username:  ");

    /* SEND CLIENT USERNAME TO THE SERVER */
    while( !bServerAccepted && gets(buf) != NULL ) // MUST HAVE IN THIS ORDER, ELSE IT HANGS ON gets()
    {
        write(socketNumber, buf, sizeof(buf));
        read(socketNumber, buf, sizeof(buf));
        if(strcmp(buf,"/server_full")==0)
        {
            printf("The server is at capacity - try again later!\n");
            close(socketNumber);
            exit(0);
        }
        else
            printf("%s", buf);
        
        bServerAccepted = true;
    }

    printf( "\nType /exit OR /quit OR /part to leave the room!\n");
    printf( "BEGIN CHATTING!\n\n");

    /* CREATE THREAD FOR READING FROM SERVER */
    if (pthread_create(&threadID, NULL, readThread, &socketNumber))
    {
        perror("CLIENT ERROR: Read thread could not be created.\n");
        exit( EXIT_FAILURE );
    }

    /* CONTINUOUSLY READ USER INPUT AND SEND MESSAGE TO SERVER, UNTIL SERVER OR USER EXITS */
    while( !bClientExit && gets(buf) != NULL )
    {
        /* IF THE USER WANTS TO EXIT */
        if(strcmp(buf,"/exit")==0 || strcmp(buf,"/quit")==0 || strcmp(buf,"/part")==0)
        {
            bClientExit = true; // sets the exit flag
        }

        write(socketNumber, buf, sizeof(buf)); // write message to server
    }

    close(socketNumber); // Close the socket
    return(0);
}

/* --------------------------- FUNCTION DEFINITIONS --------------------------- */

// IF CLIENT ENTERS CTRL+C, THEY ARE NOTIFIED THAT THEY MUST ENTER A VALID EXIT
// COMMAND IN ORDER TO CLEANLY EXIT THE PROGRAM.
void exitHandler(int sig)
{
    printf("\rType /exit OR /quit OR /part to leave the room!\n");
}

// CLIENT HANDLER THREAD FOR READING MESSAGES, SENT FROM THE SERVER TO THE CLIENT
void *readThread( void *sockNum )
{
    int *socket = (int*)sockNum;
    char receivedMessage[BUFFER_LENGTH]; // message received from server

    /* READ MESSAGES FROM THE SERVER */
    while(read(*socket, receivedMessage, sizeof(receivedMessage)) > 0)
    {
        // IF MESSAGED RECEIVED IS THE SERVER'S SIGNAL TO KILL THE CLIENT
        if(strstr(receivedMessage,"/server_closing") != NULL)
        {
            printf("The server is closing - YOU WILL BE DISCONNECTED!\n");
            close(socketNumber); // CLOSE THE SOCKET AND EXIT CLIENT APPLICATION
            exit(EXIT_SUCCESS);
        }

        printf("%s\n",receivedMessage); // ELSE PRINT MESSAGE SENT BY SERVER
        bzero(receivedMessage, sizeof(receivedMessage)); // CLEANING BUFFER
    }

    return NULL; // to make the compiler happy
}
