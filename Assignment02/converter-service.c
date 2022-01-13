/**
 * 	CPSC 441, Assignment 2, Fall 2021
 *  Written by Joshua Vanderstoop 
 * 
 * 
 * Usage: 
 *          gcc -o converter converter-service.c
 *          ./converter 
 * 
 * Program: converter-service.c
 *      - gets client command from indirection-server.c
 *      - tries to convert command into a number value
 *      - deciphers which currencies to use in conversion
 *      - converts currencies 
 *      - sends information back to indirection server 
 * 
 * Communicates with: indirection-server.c
 * acts as: server
 * uses: UDP connection 
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

#define PORTNUM 8887

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
    // To retrieve hostname
    gethostname(hostbuffer, sizeof(hostbuffer)); 

    printf("Host IP: %s\n", hostbuffer); //so the indirection server knows where to look

    //perform a handshake so the server knows that someone wants to use it
    printf("awaiting hand......\n");

    //begin to receive the client requests
    char toConvert[MAX_MESSAGE_LENGTH]; 
    char convertResult[MAX_MESSAGE_LENGTH];
    char *conversionType;
    char *numStart; 
    double cadValue = 0; 
    double convertedValue = 0;  
    int bytes, done = 0; 
    while (!done)
    {
        bzero(toConvert, MAX_MESSAGE_LENGTH); 
        bzero(convertResult, MAX_MESSAGE_LENGTH);

        //get the conversion request from the user
        bytes = recvfrom(serverfd, toConvert, MAX_MESSAGE_LENGTH, 0, client, &c); 
        while (bytes > 0 )
        {
            toConvert[bytes] = '\0'; 

            #ifdef debug
                printf("received the following form the client:\n      %s\n", toConvert);
            #endif
            if( strcmp(toConvert, "QUIT") == 0 || strcmp(toConvert, "CLOSE")==0) //client wants out
            {
                printf("client would like to quit\n"); 
                strcpy(convertResult, "Quitting, time is money"); 
                done = 1; 
            }
            else if(strcmp(toConvert, "handshake")== 0) //send instructions upon handshake
            {
                strcpy(convertResult, "An acceptable instruction is in the form of:\n     $10 CAD US or $10.25 CAD BIT\n\nThe following currencies are availible to choose from:\n     CAD to US, EU, BP, BIT"); 
            }
            else 
            {
                numStart = toConvert; 
                numStart++;
                cadValue = atof(numStart);
                conversionType = strstr(toConvert, "CAD");
                // determine the conversion type, and multiply the user number value by the conversion factors found on google
                if( strcmp(conversionType, "CAD US") == 0 )
                {
                    convertedValue = cadValue * .81; 
                }
                else if( strcmp(conversionType, "CAD EU") == 0 )
                {
                    convertedValue = cadValue * .69;
                }
                else if( strcmp(conversionType, "CAD BP") == 0 )
                {
                    convertedValue = cadValue * .59;
                }
                else if( strcmp(conversionType, "CAD BIT") == 0 )
                {
                    convertedValue = cadValue * .000015; 
                }
                else
                {
                    //garbage request 
                    strcpy(convertResult, "That command is not in the conversion database, try again\n"); 
                }
                //print the numbers into the converted result 
                snprintf(convertResult, 10, "%f", convertedValue);
            }

            #ifdef debug
                printf("Sending the response: %s to the client\n", convertResult); 
            #endif

            //send the conversion to the client, clean biffers and get next request
            sendto(serverfd, convertResult, strlen(convertResult), 0, client, c); 
            bzero(toConvert, MAX_MESSAGE_LENGTH);
            bytes = recvfrom(serverfd, toConvert, MAX_MESSAGE_LENGTH, 0, client, &c);
            bzero(convertResult, MAX_MESSAGE_LENGTH); 
        }
    }
    close (serverfd); 
    return 0; 
}