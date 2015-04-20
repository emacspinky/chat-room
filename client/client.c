/************************************************************************/ 
/*   PROGRAM NAME: client.c  (works with serverX.c)                     */ 
/*                                                                      */ 
/*   Client creates a socket to connect to Server.                      */ 
/*   When the communication established, Client writes data to server   */ 
/*   and echoes the response from Server.                               */ 
/*                                                                      */ 
/*   To run this program, first compile the server_ex.c and run it      */ 
/*   on a server machine. Then run the client program on another        */ 
/*   machine.                                                           */ 
/*                                                                      */ 
/*   COMPILE:    gcc -o client client.c -lnsl                           */ 
/*   TO RUN:     client  server-machine-name                            */ 
/*                                                                      */ 
/************************************************************************/ 

//
//  client.c
//  Client code
//
//  Joel Bierbaum - 4/17/2015
//  CS 3800 Assignment 3
//

#include <stdio.h> 
#include <sys/types.h> 
#include <sys/socket.h>  /* define socket */ 
#include <netinet/in.h>  /* define internet socket */ 
#include <netdb.h>       /* define internet socket */ 
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>
 
#define SERVER_PORT 9999     /* define a server port number */

int socketNumber = 0; // file descriptor number assigned to socket upon connection
char clientName[200]; // client's chosen name --- IS THIS BAD, SHOULD IT JUST BE AN ARRAY?
pthread_t threadID;
bool exitFlag = false;

/* --------------------------- FUNCTION PROTOTYPES --------------------------- */

void *readThread( void *ptr );
void exitHandler(int sig);
void exitChatRoom();

/* --------------------------- MAIN PROGRAM --------------------------- */

int main( int argc, char* argv[] ) 
{
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    char buf[512];
    struct hostent *hostInfo;
    bool bServerAccepted = false;
    bool bClientExit = false;

    signal(SIGINT,exitHandler);

    /* VALIDATE INITIAL ARGUMENTS -- EXECUTABLE AND HOSTNAME */
    if( argc != 2 ) 
    { 
        //printf( "Usage: %s hostname\n", argv[0] ); // Not sure what this even means
        printf("CLIENT ERROR: Invalid number of arguments!\n\n");
        exit(1);
    }

    /* GET HOST INFORMATION */
    if( ( hostInfo = gethostbyname( argv[1] ) ) == NULL )
    {
        //printf( "%s: %s unknown host\n", argv[0], argv[1] );
        printf("CLIENT ERROR: %s is an unknown host!\n\n",argv[1]);
        exit( 1 ); 
    }
    bcopy(hostInfo->h_addr_list[0], (char*)&server_addr.sin_addr, hostInfo->h_length);
 
    /* CREATE CLIENT SOCKET */
    if( ( socketNumber = socket( AF_INET, SOCK_STREAM, 0 ) ) == -1 )
    {
        perror( "CLIENT ERROR: SOCKET CREATION FAILED!\n\n" );
        exit( 1 ); 
    } 

    /* CONNECT TO SERVER SOCKET */
    if( connect( socketNumber, (struct sockaddr*)&server_addr, sizeof(server_addr) ) == -1 )
    { 
        perror( "CLIENT ERROR: connect() FAILED!\n\n" );
        exit( 1 ); 
    } 
 
    printf("CONNECTED TO CHATROOM!\n");
    printf("Input your username:  ");

    /* SEND CLIENT USERNAME TO THE SERVER */
    while( !bServerAccepted && gets(buf) != NULL ) // MUST HAVE IN THIS ORDER, ELSE IT HANGS ON gets()
    {
        strcpy(clientName, buf);
        write(socketNumber, buf, sizeof(buf));
        read(socketNumber, buf, sizeof(buf));
        if(strcmp(buf,"/server_full")==0) // MAY NOT NEED THIS...DEPENDS ON SERVER
        {
            printf("The server is at its maximum client level - try again later!\n");
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
    if (pthread_create(&threadID, NULL, readThread, &socketNumber)) // DOES THE THREAD ID EVER HAVE TO BE USED FOR ANYTHING???
    {
        perror("CLIENT ERROR: Read thread could not be created.\n");
        exit(1);
    }

    /* CONTINUOUSLY READ USER INPUT AND SEND MESSAGE TO SERVER, UNTIL SERVER OR USER EXITS */
    while( !bClientExit && gets(buf) != NULL )
    {
        if(strcmp(buf,"/exit")==0 || strcmp(buf,"/quit")==0 || strcmp(buf,"/part")==0)
        {
            bClientExit = true; // sets the exit flag
        }

        write(socketNumber, buf, sizeof(buf)); // write message to server
    }

    close(socketNumber); // Outta here
    return(0);
}

/* --------------------------- FUNCTION DEFINITIONS --------------------------- */

// IF CLIENT ENTERS CTRL+C, THEY ARE NOTIFIED THAT THEY MUST ENTER A VALID EXIT COMMAND
// IN ORDER TO CLEANLY EXIT THE PROGRAM.
void exitHandler(int sig)
{
    printf("\rType /exit OR /quit OR /part to leave the room!\n");
}

void *readThread( void *sockNum )
{
    int *socket = (int*)sockNum; // DON'T KNOW IF THIS WILL WORK OR NOT, MIGHT HAVE TO BE int *socket = (int*)sockNum
    char receivedMessage[512];
    while(read(*socket, receivedMessage, sizeof(receivedMessage)) > 0) // WHEN I HAD THIS !=0 IT WOULD PRINT BLANK LINES, CONSTANTLY >=0 FIXED THIS...WHICH MEANS THAT read() IS SPITTING ERRORS WHEN READING NOTHING...NO PROBLEM
    {
        // IF MESSAGED RECEIVED IS THE SERVER'S SIGNAL TO KILL THE CLIENT
        if(strstr(receivedMessage,"/server_closing") != NULL)
        {
            printf("The server is closing - YOU WILL BE DISCONNECTED!\n");
            close(socketNumber); // CLOSE THE SOCKET AND EXIT CLIENT APPLICATION
            exit(EXIT_SUCCESS);
        }

        printf("%s\n",receivedMessage); // ELSE PRINT MESSAGE SENT BY SERVER
        bzero(receivedMessage, sizeof(receivedMessage));
    }

    return NULL; // to make the compiler happy
}

void exitChatRoom()
{
    
}