/**
 * 	CPSC 441, Assignment 1, Fall 2021
 *  Written by Joshua Vanderstoop 
 * 
 * usage: 
 *      - gcc -o Proxy.exe proxy_vanderstoop.c
 *      - ./Proxy.exe
 **/

#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#define MAX_BUFFER_SIZE 10000
#define MAX_MESSAGE_LENGTH 10000

/* Global Variables */ 

int serverChildSocketfd;

char serverResponse[MAX_MESSAGE_LENGTH]; 
char response[MAX_MESSAGE_LENGTH] = {0}; 

int item1 = 0; //floppy
int item2 = 0; //spongebob
int item3 = 1; //curling

/* Debugging option [1 = debug, 0 = !debug] */
#define DEBUG  1 

int getRequest( char *clientrequest);
int checkBlock( char *filePath); 


int main ()
{
    struct sockaddr_in server, client; 
    int serverParentSocketfd, clientSocketfd, port;
    port = 8869; //edit to fit proxy configuration of your computer
    int c = sizeof(struct sockaddr_in); 
    
    /* Create the socket to listen for client connection */
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    // ensure the use of TCP -> SOCK_STREAM
    if ( (serverParentSocketfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("Could not create socket to listen to client");
        exit(1); 
    }

    /* bind a specific address and port to the end point */
    if( bind(serverParentSocketfd, (struct sockaddr *)&server, sizeof(struct sockaddr_in) ) == -1 )
    {
	    printf("bind() call failed!\n");
	    exit(1);
    }

    // ensure that the bind was successful and display the information to the terminal. 
    // get the host name and build the host IP to be displayed to the terminal 
    // get the host name and build the host IP to be displayed to the terminal 
    char hostbuffer[256];
    char *IPbuffer;
    struct hostent *host_entry;
    int hostname;
    // To retrieve hostname
    hostname = gethostname(hostbuffer, sizeof(hostbuffer)); 
  
    // To retrieve host information
    host_entry = gethostbyname(hostbuffer);
  
    // To convert an Internet network
    // address into ASCII string
    IPbuffer = inet_ntoa(*((struct in_addr*)host_entry->h_addr_list[0]));
  
    printf("Hostname: %s\n", hostbuffer);
    printf("Host IP: %s\n", IPbuffer);

#ifdef DEBUG
        printf("\n(LISTENING)\n"); 
#endif

    // listen for incomming connections for clients. 
    if (listen(serverParentSocketfd, 5) == -1)
    {
        printf("listen() fail");
        exit(1); 
    } 
    for (;;)
    {  
        // accept an incoming connection, maybe done through telnet [ip] [port] on the other window 
        if ( serverChildSocketfd = accept(serverParentSocketfd, (struct sockaddr *)&client, (socklen_t *) &c ) == -1)
        {
            printf("could not accept a connection\n");
            exit(1); 
        }

        int pid = fork(); 
        if (pid < 0)
        {
            printf("fork failed\n"); 
        }
        else if (pid == 0)
        {
            close(serverParentSocketfd);
            //the connection is accepted, parent isnt needed  
            char clientRequest[MAX_MESSAGE_LENGTH] = {0}; 
            int bytes; 

            // get the GET request from the client
            bytes = recv(serverChildSocketfd, clientRequest, MAX_MESSAGE_LENGTH, 0); 
            #ifdef DEBUG
                printf("        ------The client requested the following----- \n\n%s\n", clientRequest);
            #endif

            // create a message to send back as the server response
            if ( getRequest(clientRequest) == -1)
            {
                close(serverChildSocketfd);
                exit(1); 
            }
        }
        else 
        {
            close(serverChildSocketfd);
        }
    }
    close (serverChildSocketfd);
    return 1; 
}


/**
 * checkBlock
 * return 1 to block 
 * checks the filepath for the flagged strings based on item[n] values 
 * 
 */
int checkBlock( char* filePath)
{   
    char* block = NULL; 
    if (item1 == 1) 
    { // check for floppy in filePath
        block = strstr(filePath, "Floppy");
        if (block != NULL)
        {
            return 1; 
        }
    }
    if (item2 == 1)
    {   //check for SpongeBob in the filePath 
        block = NULL;
        block = strstr(filePath, "SpongeBob");
        if (block != NULL)
        {
            return 1; 
        }
    }
    if (item3 == 1)
    {   //check for curling in the filePath 
        block = NULL;
        block = strstr(filePath, "curling");
        if (block != NULL)
        {
            return 1; 
        }
    }
    
    return 0; 
}


int getRequest ( char *clientRequest)
{
    char buf2[MAX_BUFFER_SIZE];
	struct sockaddr_in sin;
	int clientChildSocket; /* request destination */
	int n;
	struct hostent *hotServer;
	char *url;
	char host[128];
	char *h,*rest;
	int portnum;
	extern int h_errno;
	char cmd[16]; 
	int i;

	/**
     * Parse the clientRequest to get:
     *  - command
     *  - hostname
     *  - path
     *  - url
     *  - rest of the request
     */
	for(i=0;i<15;i++)
		if(clientRequest[i] && (clientRequest[i]!=' ')) 
        {
			cmd[i]=(char)toupper((int)clientRequest[i]);
		} else break;
	cmd[i]='\0';
    for(url=clientRequest;*url && (*url!='/');url++);
	url+=2;
	h=host;
	for(;*url&&(*url!=':')&&(*url!='/');url++) *(h++)=*url;
	*h='\0';
	if(*url==':') 
    {
		portnum = atoi(url+1);
		for(;*url!='/';url++);
	} 
    else 
    {
		portnum = 80;
	}
	for(rest=url;*rest && (*rest!='\n');rest++);
	if(*rest){ *rest='\0'; rest++; }
    // remove " HTTP/1.1 " form the path
    char filePath[100] = {"\0"};
    for(int i = 0; i < strlen(url)-10; i++)
    {
        filePath[i] = *(url+i); 
    }
    // ensure the first line ends in \n 
    char* newline = "\n"; 
    strcpy(buf2, clientRequest); 
    strcat(buf2, newline); 

    if (strcmp(cmd, "GET")==0)
    { // need to make sure the url doesnt contain something blocked. 
        #ifdef DEBUG
            printf("the command found is: %s\n", cmd);
            printf("The hostname is: %s\n", host);
            printf("The path is: %s\n", filePath);
            printf("The url is: %s\n", url); 
            printf("The rest is: %s\n", rest); 
        #endif
            
    }

    //create the socket to the server (acts as a client) 
    if ((clientChildSocket = socket(PF_INET, SOCK_STREAM, 0)) ==-1) {
		printf("in getRequest(): socket failed");
        return -1;  
	}

    // set up sin using the hostname found in parsing 
    hotServer = gethostbyname(host); 
    sin.sin_family = AF_INET;
	bcopy(hotServer->h_addr,&sin.sin_addr,sizeof(struct in_addr)) ;
	sin.sin_port = htons((u_short) portnum) ;

    //use sin to connect the socket and the server 
	if(connect(clientChildSocket, (struct sockaddr *) &sin, sizeof(struct sockaddr_in)) ==-1) 
    {
        printf("in getRequest(): could not connect to server");
        close(clientChildSocket);
        return -1;
    }

    #ifdef DEBUG
        printf("Connected to: %s , on port: %d\n",host,portnum);
        printf("The message being sent is:\n\n%s%s\n", buf2, rest); 
    #endif
    // if checkblock returns 1, the GET request will be replaced with a hijacked request for the error page 
    if (checkBlock(clientRequest) == 1)
    {
        strcpy(buf2, "GET http://pages.cpsc.ucalgary.ca/~carey/CPSC441/ass1/error.html HTTP/1.1\r\n"); 
    }

    //send the information pulled from the GET request to the server
    if ( send(clientChildSocket, buf2, strlen(buf2), 0) == -1)
    {
        printf("in getRequest(): sending failed"); 
        close(clientChildSocket);
        return -1;
    }
    if (strlen(rest)> 0) //rest holds the rest of the information following the initial GET line
    {   //send the rest 
        if ( send(clientChildSocket, rest, strlen(rest), 0) == -1)
        {
            printf("in getRequest(): sending failed"); 
            close(clientChildSocket);
            return -1;
        }
    }

    #ifdef DEBUG
        printf("Message sent\n"); 
    #endif

    for(;;)
    // read the servers response into the global variable serverResponse  
    {
        bzero(serverResponse, MAX_MESSAGE_LENGTH); 
        int bytes = recv(clientChildSocket, serverResponse, MAX_MESSAGE_LENGTH, 0); 
        if (checkBlock(serverResponse) == 1)
        { //if what the server has sent has any blocked information, hijack the request again
            printf("gotcha\n"); 
            strcpy(buf2, "GET http://pages.cpsc.ucalgary.ca/~carey/CPSC441/ass1/error.html HTTP/1.1\r\n"); 
            int i = 0; 
            strcat(buf2, rest); 
            i = getRequest(buf2); 
            bzero(rest, MAX_MESSAGE_LENGTH); 
            bzero(buf2, MAX_MESSAGE_LENGTH); 
            return 1; 
        } // if there is no hijack, send as normal
        else if ( send(serverChildSocketfd, serverResponse, bytes , 0) ==-1)
        {
            printf("sending the server response has failed\n");
            close(serverChildSocketfd);
            exit(1); 
        }
    }
    //close the socket 
    close (clientChildSocket); 
    return 1;
}
