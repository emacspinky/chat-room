
 
#include <stdio.h> 
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h> 
#include <sys/socket.h>  /* define socket */ 
#include <netinet/in.h>  /* define internet socket */ 
#include <netdb.h>       /* define internet socket */ 
#include <string.h>

#define SERVER_PORT 2015   /* define a server port number */
int sd; 
char nickName[100]; //STORES CLIENT NAME
pthread_t tid;

//READs THREAD RECEIVED FROM THE SERVER
//AND PRINTS TO THE SCREEN.

void* readerThread(void* arg)
{
  int* socket = (int*)arg;
  char buf[512]; 
  while(read(*socket, buf, sizeof(buf)) != 0)
  {
    //KILL CLIENT
    if(strcmp(buf,"QUIT")==0)
    {
       close(sd);
       exit(0);
    }
    printf("%s\n",buf);
  }
}

//SIGNAL HANDLER -> IF CTRL+C IS PRESSED
//NOTIFIES USER TO USE /EXIT, /QUIT OR /PART TO QUIT PROGRAM.

void quitBlock(int sig)
{
  printf("Please type /exit, /quit or /part to exit the chatroom.\n");
}



//CONNECTS TO THE SERVER USING THE SERVER_PORT, 
//CREATES THE READING THREAD, AND CONSTANTLY SENDS 
//THE MESSAGE THE USER SENT TO THE SERVER.

int main( int argc, char* argv[] ) 
{ 
  int done = 0;
  int quit = 0;
  struct sockaddr_in server_addr = { AF_INET, htons( SERVER_PORT ) }; 
  char buf[512]; 
  struct hostent *hp; 
 
  //GET THE HOST 
  printf("Enter the hostname: ");
  
  if(gets(buf) == EOF)
  {
    perror("client: hostname failed");
    exit( 1 );
  }
  if( ( hp = gethostbyname( buf  ) ) == NULL ) 
  { 
    printf( "%s: %s unknown host\n", argv[0], argv[1] ); 
    exit( 1 ); 
  } 
  bcopy( hp->h_addr_list[0], (char*)&server_addr.sin_addr, hp->h_length ); 
 
  //CREATE A SOCKET 
  if( ( sd = socket( AF_INET, SOCK_STREAM, 0 ) ) == -1 ) 
  { 
    perror( "client: socket failed" ); 
    exit( 1 ); 
   } 
 
    //CONNECT A SOCKET  
  if( connect( sd, (struct sockaddr*)&server_addr,sizeof(server_addr) ) == -1 ) 
  { 
    perror( "client: connect FAILED:" ); 
    exit( 1 ); 
  } 
 
  printf("connect() successful! will send a message to server\n"); 
  printf("Input a nickname:\n" );

  //REPLACES THE CTRL+C SIGNAL HANDLER
  signal(SIGINT,quitBlock);
	
  //TAKE THE USER NICKNAME AND WRITES IT TO THE SERVER
  //(ONLY RUNS ONCE)
  while( !done && gets(buf) != EOF ) 
  { 	
    strcpy(nickName,buf);
    write(sd, buf, sizeof(buf)); 
    read(sd, buf, sizeof(buf)); 
    printf("%s\n", buf);
    if(strcmp(buf,"The Server has reached the 10 client limit.\n")==0)
    {
      close(sd);
      exit(0);
    }	  
    done = 1;
  } 
		
  //CREATES THE THREAD TO READ FROM THE SERVER
  if(pthread_create(&tid, NULL, readerThread, &sd)!=0)
  {
    perror("Thread creation failed.\n");
    exit(1);
  }
	
  //CONSTANTLY SENDS THE USER-INPUTTED MESSAGES TO THE SERVER 
  //UNTIL /EXIT, /QUIT, OR /PART IS PRESSED
  while(!quit && gets(buf) != EOF) 
  { 	
    if(strcmp(buf,"/exit")==0 || strcmp(buf,"/quit")==0 
                              || strcmp(buf,"/part")==0)
    {
      quit =1;
    }
    write(sd, buf, sizeof(buf)); 
  } 
  close(sd); //CLOSES THE SOCKET
  return(0); 
} 
