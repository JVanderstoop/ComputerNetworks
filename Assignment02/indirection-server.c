/**
 * 	CPSC 441, Assignment 2, Fall 2021
 *  Written by Joshua Vanderstoop 
 * 
 * Usage:
 *          first, run micro-services, then configure which IP address they are listening on
 *          this will be displayed on their respective terminals
 *          if the host includes 'csx', ensure that the IP address that is uncommented below ends in '25'
 *          if the host includes 'csx3', ensure the IP adress that is uncommented below ends in '27'
 *          do this for EVERY service and ensure to change the clearly labelled #defined options
 * 
 * 
 *          gcc -o indirection indirection-server.c
 *          ./indirection
 * 
 * Program: indirection-server.c
 *      - connects to already running microservices
 *              - knows IP'server and ports to connect to
 *      - gets user command from client.c 
 *      - processes command, finds right service, send/gets information from server
 *      - sends information back to the client 
 * 
 * Communicates with: client.c
 * acts as: server
 * uses: TCP connection
 * 
 * Communicates with: converter-service.c
 * acts as: client
 * uses: UDP connection
 * 
 * Communicates with: translator-service.c
 * acts as: client
 * uses: UDP connection
 * 
 * Communicates with: voting-service.c
 * acts as: client
 * uses: UDP connection
 **/


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
#include <time.h>
#include <signal.h>
#include <string.h>

#define debug 1

#define MAX_MESSAGE_LENGTH 1000

#define PORTNUM_CONVERSION 8887
#define CONVERSIONIP "136.159.5.27"
//#define CONVERSIONIP "136.159.5.25"

#define PORTNUM_TRANSLATOR 8888
#define TRANSLATORIP "136.159.5.27"
//#define TRANSLATORIP "136.159.5.25"

#define PORTNUM_VOTING 8889
//#define VOTINGIP "136.159.5.27"
#define VOTINGIP "136.159.5.25"


//functions 
int decodeRequest(char clientRequest[]); 
void server_client_communication(int clientSocketfd, char *microServiceIP, int portnum); 


int main()
{
    //create random port to listen on. 
    int randoPort; 
    srandom(time(NULL));
    randoPort = random()%1000 + 8000;
    printf("indirection server listening on port %d\n", randoPort);


    struct sockaddr_in server, client; 
                    // server socket talks to the web browser
                    // client socket talks to the web server 
    int serverParentSocketfd, clientSocketfd; 
    /* Create the socket to listen for client connection */
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(randoPort);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    // ensure the use of TCP -> SOCK_STREAM
    if ( (serverParentSocketfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("Could not create socket to listen to client\n");
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
    char hostbuffer[256];
    char *IPbuffer;
    struct hostent *host_entry;
    int hostname;
  
    // get the hostname/IP that the server is running on to confirm the client will connect properly
    hostname = gethostname(hostbuffer, sizeof(hostbuffer)); 
    host_entry = gethostbyname(hostbuffer);
    IPbuffer = inet_ntoa(*((struct in_addr*)host_entry->h_addr_list[0]));
    printf("Host IP: %s\n", IPbuffer);

    // listen for incomming connections for clients. 
    if (listen(serverParentSocketfd, 5) == -1)
    {
        printf("listen() fail\n");
        exit(1); 
    }

    
    int c = sizeof(struct sockaddr_in);
    
    for (;;)
    {

        #ifdef debug
            printf("awaiting a connection....\n"); 
        #endif
        // accept an incoming connection
        clientSocketfd = accept(serverParentSocketfd, (struct sockaddr *)&client, (socklen_t *) &c );
        if ( clientSocketfd == -1)
        {
            printf("could not accept a connection\n");
            exit(1); 
        }
	    else printf("Connection accepted!\n");


        int pid = fork(); 

        if (pid < 0)
        {
            printf("fork failed\n"); 
        }
        else if (pid == 0)
        {     
            //parent no longer needed    
	        close(serverParentSocketfd);
            char clientRequest[MAX_MESSAGE_LENGTH] = {0}; 
            int bytes =0; 

            #ifdef debug
                printf("awating a command from the client.\n\n");
            #endif

            // get the initial command from the client
            bytes = recv(clientSocketfd, clientRequest, MAX_MESSAGE_LENGTH, 0);
            while ( bytes > 0)
            {         
                #ifdef debug
                    printf("A command '%s' with %d bytes has come through\n",clientRequest, bytes);
                #endif

                //decide the clients requested service using decodeRequest
                int toWhom = decodeRequest(clientRequest); 
                if ( toWhom == -1 )
                {
                    char *error = "Sorry, the server could not redirect you. Try another command\n"; 
                    send(clientSocketfd, error, strlen(error), 0); 
                }
                else if( toWhom == 1) //client wants to translate something
                {
                    server_client_communication(clientSocketfd, TRANSLATORIP, PORTNUM_TRANSLATOR); 
                }
                else if (toWhom == 2) //client wants to convert money
                {
                    server_client_communication(clientSocketfd, CONVERSIONIP, PORTNUM_CONVERSION); 
                }
                else if (toWhom == 3) // client wants to vote 
                {
                    server_client_communication(clientSocketfd, VOTINGIP, PORTNUM_VOTING); 
                }
                else if (toWhom == 4) //client wants to disconnect
                {
                    close(clientSocketfd); 
                    exit(0); 
                }
                bzero(clientRequest, MAX_MESSAGE_LENGTH);

		        // get the next command from the client
		        bytes = recv(clientSocketfd, clientRequest, MAX_MESSAGE_LENGTH, 0);

            }
            close(clientSocketfd); 
            exit(0); 
        }
        else 
        {
            close(clientSocketfd); 
        }
    }
    close (serverParentSocketfd); 
    return 1; 
}

/**
 * decodeRequest
 *      accepts the client request 
 *      finds the command at the begining of the request
 *      directs the client to the proper microservice 
 * 
 * @param clientRequest
 *      contains the command from the client
 */
int decodeRequest(char  clientRequest[])
{
    char *director = NULL;
    if ( (director = strstr(clientRequest, "TRANSLATE")) != NULL )
    {
        return 1; 
    }
    else if ( (director = strstr(clientRequest, "CONVERT")) != NULL )
    {
        return 2; 
    }
    else if ( (director = strstr(clientRequest, "VOTE")) != NULL )
    {
        return 3; 
    }
    else if( (director = strstr(clientRequest, "QUIT")) != NULL || (director = strstr(clientRequest, "CLOSE")) != NULL) 
    {
        return 4; 
    }
    else 
    {
        return -1; 
    }
}

/**
 * server_client_communication
 *      sends information between the client and the micro-service specified by decodeRequest
 * @param clientSocketfd is the location to send server replies to 
 * @param microServiceIP is the IP of the micro-service
 * @param portnum        is the port that the micro-service runs on 
 */
void server_client_communication(int clientSocketfd, char* microServiceIP, int portnum)
{
    #ifdef debug
        printf("\nCurrently creating a socket for the micro-service\n");
    #endif

    //create a connection to the micro-service using UDP
    struct sockaddr_in si_server;
    struct sockaddr *server;
    int serverfd, length = sizeof(si_server);
    //specify UDP connection
    if ((serverfd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
    {
	    printf("Could not setup a socket!\n");
	    return;
    }

    #ifdef debug
        printf("Socket created\n"); 
    #endif
      
    memset((char *) &si_server, 0, length);
    si_server.sin_family = AF_INET;
    si_server.sin_port = htons(portnum);
    server = (struct sockaddr *) &si_server;
    //turn the microServiceIP to a address structure
    if (inet_pton(AF_INET, microServiceIP, &si_server.sin_addr)==0)
    {
	    printf("inet_pton() failed\n");
	    return;
    }
    //send the handshake the UDP servers wait for 
    char *handshake = "handshake"; 
    if ( sendto(serverfd, handshake, strlen(handshake), 0, server, length) == -1 ) 
    {
        printf("sending client request to the micro-server failed\n"); 
        return; 
    }


    int done = 0, fromServer=0, quit = 0; 
    char message[MAX_MESSAGE_LENGTH]; 
    //get the initial instructions from the translator
    if ((fromServer=recvfrom(serverfd, message, MAX_MESSAGE_LENGTH, 0, server, &length))==-1)
    {
        printf("Error reading message from micro-service to indirection server\n");
        done = 1;
    }
    //ensure it is what we want to send
    #ifdef debug
        printf("The message sent %i bytes from the micro-service:\n      %s\n",fromServer, message); 
    #endif
    //send to the client using TCP connection passed into the function
    if (send(clientSocketfd, message, strlen(message), 0) == -1)
    {
        printf("the message could not be sent to the client from the indirection server\n"); 
        done = 1; 
    }

    #ifdef debug
        printf("Sent %i bytes to the client:\n      %s\n", fromServer, message); 
    #endif

    //now, read client requests and send them on to the micro-service, then get the response from the service. 
    while (!done)
    {
        bzero(message, MAX_MESSAGE_LENGTH); //clear message
        //get next request from the client in TCP
        if ( recv(clientSocketfd, message, MAX_MESSAGE_LENGTH, 0) < 0)
        {
            printf("could not get the client request following instructions\n"); 
            done = 1; 
        }

        #ifdef debug
            printf("client request is as follows: \n      %s\n", message); 
        #endif

        //send the request to the server in UDP
        if ( sendto(serverfd, message, strlen(message), 0, server, length) == -1 ) 
        {
            printf("sending client request to the micro-server failed\n"); 
            done = 1; 
        }

        //if the user elects to quit, this will end both indirection loop and also close down the current connection in the server 
        if (strcmp(message, "QUIT") == 0)
        {
            done = 1; 
            quit = 1; 
        }

        if (strcmp(message, "CLOSE")==0)
        {
            close(serverfd);
            return; 
        }

        //message now will contain the server response, so must be cleaned
        bzero(message, MAX_MESSAGE_LENGTH); 
        //get the response form the server in UDP
        if (fromServer = recvfrom(serverfd, message, MAX_MESSAGE_LENGTH, 0, server, &length) == -1)
        {
            printf("could not read response from the micro-service\n");
            done = 1; 
        }
        //confirm the user has exited the service, prompt for further use or close the whole connection
        if(quit)
        {
            strcat(message, "\nPlease use another server or enter CLOSE to close your connection"); 
        }
        

        //send the response to the client in TCP
        if (send(clientSocketfd, message, strlen(message), 0) == -1)
        {
            printf("the message could not be sent to the client from the indirection server\n"); 
            done = 1; 
        }
        //clean buffer
        bzero(message, MAX_MESSAGE_LENGTH);
        
    }
    close(serverfd); 
}
 

