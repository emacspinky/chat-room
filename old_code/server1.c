
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>  /* define socket */
#include <netinet/in.h>  /* define internet socket */
#include <netdb.h>       /* define internet socket */
#include <pthread.h>
#include <string.h>
#include <signal.h>

int sd;
int SERVER_PORT = 2015;        /* DEFINE A SERVER PORT NUMBER */
pthread_mutex_t lock;
pthread_mutex_t writeLock;
int counter = 0;
pthread_t tids[10];
int clients[10];
int quit = 0;
char quitMessage[]="Exiting in 10 secs...";

struct sockaddr_in server_addr;


//FUNCTION IS CALLED WHEN CTRL+C HAS BEEN PRESSED.
//TELLS ALL OF THE CLIENTS THAT THE SERVER WILL BE SHUT DOWNN IN 10 SECONDS.
//AT THE END OF 10 SECONDS IT CLOSES ALL OPEN CLIENTS AND EXITS

void quitSignal(int sig)
{
  int i =0;
  printf("%s\n",quitMessage);

  for(i = 0;i<counter;i++)
  {
    write(clients[i],quitMessage,sizeof(quitMessage));
  }
  pthread_mutex_lock(&writeLock);
  sleep(10);
  for(i=0;i<counter;i++)
  {
    write(clients[i],"QUIT",5);
    close(clients[i]);
    pthread_kill(tids[i],0);
  }
  close(sd);
  pthread_mutex_unlock(&writeLock);
  unlink(server_addr.sin_addr);
  exit(0);  
}


//THREAD TO SIMULATE EACH CLIENT. TAKE THE CLIENTS NAME AND SENDS THE 
//MESSAGE TO ALL OF THE OTHER CLIENTS.

void *clientThread(void *var)
{
  char buf[512], *host;
  char greeting1[100];
  char greeting2[] = " has joined the server.";
  int k = 0;
  int *socket = (int*)var;
  int i = 0;
  char clientName[100];
  int clientIndex= 0;
  int done=0;
  int clientSocket = *socket;
	
  /* DATA TRANSFER ON CONNECTED SOCKET NS */
	
  //Get client name
  while(!done && (k = read(clientSocket, buf, sizeof(buf))) != 0)
  {    
    pthread_mutex_lock(&writeLock);
    printf("SERVER RECEIVED: %s\n", buf);
    strcpy(greeting1,buf);
    strcat(greeting1,greeting2);

    for(i=0;i<counter;i++)
    {
      write(clients[i], greeting1, k);
    }
    //ONLY RETRIEVE THE NICKNAME ONCE
    strcpy(clientName,buf);
    done = 1;
    pthread_mutex_unlock(&writeLock);
  }
	
	
  //READ IN OTHER MESSAGE AND PROPAGATE IT TO OTHER CLIENTS.
  while((k = read(clientSocket, buf, sizeof(buf))) != 0)
  {
     pthread_mutex_lock(&writeLock);
    char temp[100];
    printf("%s: %s\n", clientName,buf);
    //tell other clients if one is leaving
    if(strcmp(buf,"/exit")==0 || strcmp(buf,"/quit")==0 || 
       strcmp(buf,"/part")==0)
    {
      strcpy(temp, clientName);
      strcat(temp, " is leaving the chatroom");
    }
    else
    {
      strcpy(temp, clientName);
      strcat(temp, ": ");
      strcat(temp, buf);
    }
 
    //WRITES THE MESSAGE TO ALL OTHER CLIENTS
    for(i=0;i<counter;i++)
    {
      if(clients[i] != clientSocket)
      {
        write(clients[i], temp, sizeof(temp));
      }
    }
    pthread_mutex_unlock(&writeLock);
  }

  //REMOVES ITSELF FROM THE LIST OF CLIENTS
  pthread_mutex_lock(&writeLock);
  for(i = 0;i<counter;i++)
  {
    if(clients[i] == clientSocket)
    {
      clientIndex=i;
    }	
  }
  if(clientIndex != counter-1)
  {
    for(i=clientIndex;i<counter-1;i++)
    {
      clients[i] = clients[i+1];
      tids[i]=tids[i+1];
    }
  }
  //DECREMENTS THE GLOBAL CLIENT COUNTER
  counter--;
  pthread_mutex_unlock(&writeLock);
  close(clientSocket);
}


//SERVER SETS UP THE SOCKET AND ENSURES THAT A PORT IS OPEN TO CONNECT TO.
//IT ENSURES THAT THE SOCKET IS BOUND AND WILL ACCEPT NEW CLIENTS. 

int main()
{
  int ns,sd;
  int i = 0;
  struct sockaddr_in client_addr = { AF_INET };
  int client_len = sizeof( client_addr );
  char buf[512], *host;
  int add =0;

  server_addr.sin_family=AF_INET;
  server_addr.sin_port=htons(SERVER_PORT);

  /* CREATE A STREAM SOCKET */
  if( ( sd = socket( AF_INET, SOCK_STREAM, 0 ) ) == -1 )
  {
    perror( "server: socket failed" );
    exit( 1 );
  }
    
  /* BIND THE SOCKET TO AN INTERNET PORT */
  //THIS OPENS AN AVAILABLE SERVER PORT
  if( bind(sd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1 )
  {
    int i = 2000;
    for(;i<=65535;i++)
    {
      SERVER_PORT = i;
      server_addr.sin_port = htons( SERVER_PORT );

      if( bind(sd, (struct sockaddr*)&server_addr, sizeof(server_addr)) != -1 )
      {			 
         i = 66000;
      }
    }
    if(i == 65536)
    {
      perror( "server: bind failed" );
      exit( 1 );
    }
  }
  printf("Port is: %d\n",SERVER_PORT);
  /* LISTEN FOR CLIENTS */
  if( listen( sd, 1 ) == -1 )
  {
    perror( "server: listen failed" );
    exit( 1 );
  }

  printf("SERVER is listening for clients to establish a connection\n");
  //HANDLES IF CTRL+C IS PRESSED
  signal(SIGINT,&quitSignal);  
    
  //ACCEPTS ANY CLIENT THAT IS TRYING TO CONNECT TO THE SERVER
  while(!quit && ( ns = accept( sd, (struct sockaddr*)&client_addr,
                       &client_len ) ) != -1 )
  {
    pthread_t* temp = &tids[counter];
	
    //CRITICAL POINT
    pthread_mutex_lock(&writeLock);
    if(counter < 10) 			//IF THE SERVER IS OPEN FOR CLIENTS
    {
      clients[counter++]=ns;
      add =1;
    }
    else 									//IF THE SERVER HIT THE 10 CLIENT LIMIT
    {
       write(ns,"The Server has reached the 10 client limit.\n",48);
       close(ns);
    }
    pthread_mutex_unlock(&writeLock);
    if(add)
    {   
      if( pthread_create(temp, NULL, clientThread, &ns) != 0)
      {
	perror("Thread creation failed.\n");
	exit(1);
      }
      add =0;
    }
    printf("accept() successful.. a client has connected! waiting for a message\n");
  }		
  //PRINTS IF THE SERVER DID NOT ACCEPT THE CLIENT
  if( ns == -1) 
  {
    perror( "server: accept failed" );
    exit( 1 );
  }
  for(i = 0; i<counter; i++)
  {
    pthread_join(tids[counter],NULL);	
  }
  close(sd);
  unlink(server_addr.sin_addr);
  return(0);
}

