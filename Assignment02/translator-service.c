/**
 * 	CPSC 441, Assignment 2, Fall 2021
 *  Written by Joshua Vanderstoop
 * 
 * Usage: 
 *          gcc -o translator translator-service.c
 *          ./translator
 * 
 * Program: translator-service.c
 *      - gets client command from indirection-server.c
 *      - tries to translate the command from english to french
 *      - sends information back to indirection server 
 * 
 * Communicates with: indirection-server.c
 * acts as: server
 * uses: UDP connection 
 * 
 **/

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>

#define PORTNUM 8888

#define MAX_MESSAGE_LENGTH 1000

#define debug 1

int main()
{
    struct sockaddr_in si_server, si_client;
    struct sockaddr *server, *client;
    int serverfd, c=sizeof(si_server);

    //set up the server socket information 
    si_server.sin_family = AF_INET;
    si_server.sin_port = htons(PORTNUM);
    si_server.sin_addr.s_addr = htonl(INADDR_ANY);
    server = (struct sockaddr *) &si_server;
    client = (struct sockaddr *) &si_client;

    //create a socket to send and receive information with a client
    if ( (serverfd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1 )
    {
	    printf("socket could not be created\n");
	    return 1;
    }

    //bind the socket to a port
    if ( bind(serverfd, server, sizeof(si_server)) == -1 )
    {
	    printf("Bind failed"); 
	    return 1;
    }

    #ifdef debug
        printf("Bind Successful\n"); 
    #endif

    // ensure that the bind was successful and display the information to the terminal. 
    // get the host name and build the host IP to be displayed to the terminal 
    // get the host name and build the host IP to be displayed to the terminal 
    char hostbuffer[256];
    char *IPbuffer;
    struct hostent *host_entry;
    int hostname;
    // To retrieve hostname
    gethostname(hostbuffer, sizeof(hostbuffer)); 

    printf("Host IP: %s\n", hostbuffer); //so the indirection server knows where to look
    
    //perform a handshake with client
    printf("awaiting hand......\n");

    //begin to translate the client requests
    char englishRequest[MAX_MESSAGE_LENGTH]; 
    char frenchReply[MAX_MESSAGE_LENGTH]; 
    int bytes, done = 0; 
    while (!done)
    {
        bzero(englishRequest, MAX_MESSAGE_LENGTH); 
        bzero(frenchReply, MAX_MESSAGE_LENGTH);

        //get the english word
        bytes = recvfrom(serverfd, englishRequest, MAX_MESSAGE_LENGTH, 0, client, &c); 
        while (bytes > 0)
        {
            englishRequest[bytes] = '\0'; 

            #ifdef debug
                printf("The server will now convert %s to french\n", englishRequest); 
            #endif

            // determine the word that is to be translated, and set the french reply to the corresponding translation
            if( strcmp(englishRequest, "HELLO") == 0 )
            {
                strcpy(frenchReply, "bonjour"); 
            }
            else if( strcmp(englishRequest, "COMPUTER") == 0 )
            {
                strcpy(frenchReply, "ordinateur"); 
            }
            else if( strcmp(englishRequest, "SCIENCE") == 0 )
            {
                strcpy(frenchReply, "science"); 
            }
            else if( strcmp(englishRequest, "IS") == 0 )
            {
                strcpy(frenchReply, "est"); 
            }
            else if( strcmp(englishRequest, "FUN") == 0 )
            {
                strcpy(frenchReply, "amusant"); 
            }
            else if( strcmp(englishRequest, "QUIT") == 0 || strcmp(englishRequest, "CLOSE")==0)
            {
                printf("client would like to quit\n"); 
                strcpy(frenchReply, "Quitting, au revoir"); 
                done = 1; 
            }
            else if(strcmp(englishRequest, "handshake")==0) //send instructions upon handshake
            {
                strcpy(frenchReply, "The following words are available to be translated:\n     HELLO\n     COMPUTER\n     SCIENCE\n     IS\n     FUN\n");
            }
            else  //user sent some random garbage that cant be translated
            {
                strcpy(frenchReply, "That word is not in the translation database, try again\n"); 
            }

            #ifdef debug
                printf("Sending the response:   '%s'   to the client\n", frenchReply); 
            #endif

            //send the french translation to the client, clean buffers and get a new request 
            sendto(serverfd, frenchReply, strlen(frenchReply), 0, client, c); 
            bzero(englishRequest, MAX_MESSAGE_LENGTH); 
            bytes = recvfrom(serverfd, englishRequest, MAX_MESSAGE_LENGTH, 0, client, &c);
            bzero(frenchReply, MAX_MESSAGE_LENGTH); 
        }
    }
    close (serverfd); 
    return 0; 
}