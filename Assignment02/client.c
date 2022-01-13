/**
 * 	CPSC 441, Assignment 2, Fall 2021
 *  Written by Joshua Vanderstoop 
 * 
 * Usage:  
 *          when the indirection-server.c program is running, the first few commands will show an IP address and a port number.
 *          use them below to connect to the server 
 *          gcc -o client client.c
 *          ./client [IP] [port]
 * 
 * Program: client.c
 *      - uses microservices
 *      - gets user commands, sends them to the indirection server 
 * 
 * communicates with: indirection-server.c
 * acts as: client
 * uses: TCP Connection
 **/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <signal.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>


#define MAX_MESSAGE_LENGTH 1000

#define debug 1



int main (int argc, char *argv[])
{

    struct sockaddr_in server;
    int clientSocketfd, portnum;

    // get the command line args and translate them into the TCP server address and port number provided in the terminal of the server
    char *inet_IP;
    if( argc !=3)
    {   
        printf("Incorrect number of command line arguments. try this:\n./client [IP] [port]\n");
        exit(0); 
    }
    else 
    {   //the user inputed a nice command line arg set
        portnum = atoi(argv[2]);
        inet_IP = argv[1]; 
        printf("now trying to contact %s on port %i", inet_IP, portnum); 
    }



    //begin to create socket to speak to server on specified address/port
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(portnum);
    server.sin_addr.s_addr = inet_addr(inet_IP);
    //create a socket
    if ( (clientSocketfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("Could not create socket\n");
        return 1; 
    }

    #ifdef debug 
        printf("The socket has been created\n"); 
    #endif
    //connect the socket to the specified server 
	if (connect(clientSocketfd , (struct sockaddr *)&server , sizeof(struct sockaddr_in)) == -1)
	{
		puts("connection error\n");
		return 1;
	}   

    #ifdef debug
        printf("you are connected\n"); //yay
    #endif

    //begin to prompt the user for information
    printf("Please enter your command now. Your options are TRANSLATE, CONVERT, VOTE, QUIT[exits current service], or CLOSE[closes client]\n\n"); 
    char *clientCommand = malloc(MAX_MESSAGE_LENGTH);
    if (clientCommand == NULL) 
    {
        printf("No memory\n");
        return 1;
    }
    char serverReply[MAX_MESSAGE_LENGTH];
    int bytes, done = 0, voteMode=0; 
    int encryptionKey = 0, clientVote=0;
    while (!done)
    {
        bzero(clientCommand, MAX_MESSAGE_LENGTH);
        bzero(serverReply, MAX_MESSAGE_LENGTH); 

        //get the users command from the terminal, ensure it is in the form of a string
        fgets(clientCommand, MAX_MESSAGE_LENGTH, stdin);
        if ((strlen(clientCommand) > 0) && (clientCommand[strlen (clientCommand) - 1] == '\n'))
        {
            clientCommand[strlen (clientCommand) - 1] = '\0';
        }

        //if the users last command was to VOTE, expect an encryption key from the server
        if(voteMode==1)
        {
            #ifdef debug
                printf("the client voted for: %i, and the encryption key is: %i", atoi(clientCommand), encryptionKey);
            #endif
            //the client vote contains a number, so encrypt using the key and then ensure the command is sent along
            clientVote = encryptionKey * atoi(clientCommand); 
            sprintf(clientCommand, "%i", clientVote);
            voteMode=0; //cant vote twice, so turn it off so other instructions are untouched
        }

        //the the command is VOTE
        if (strcmp(clientCommand, "VOTE")==0)
        {
            voteMode = 1;   //turn on vote protocal to encrypt votes 
        }
        else 
        {
            voteMode = 0;   //turn the protocol off
        }
                  
        //send the client command to the server 
        bytes = send(clientSocketfd, clientCommand, strlen(clientCommand), 0);
        if ( bytes < 0)
        {
            printf("Sending failed\n");
            done = 1; 
        }

        //if the client is done, close the connections
        if(strcmp(clientCommand, "CLOSE") ==0)
        {
            close(clientSocketfd); 
            return 1; 
        }
	    #ifdef debug
            printf("sent %d bytes '%s' to server...\n", bytes, clientCommand);
        #endif

    
        //get the server response
	    bytes = recv(clientSocketfd, serverReply, MAX_MESSAGE_LENGTH, 0);
        if ( bytes < 0)
        {
            printf("Reply could not be processed\n"); 
            done = 1; 
        }

        #ifdef debug
	        printf("Got %d bytes back from server! the message will display below:\n", bytes);
        #endif

        //if the client had any references to Voting in the last command, find an encryption key sent from the server
        if (voteMode==1)
        {
            #ifdef debug
                printf("encryption key is: %c\n", serverReply[0]);
            #endif
            encryptionKey = serverReply[0] - '0'; 
            serverReply[0] = ' '; //hide the key from the client.  

            if (encryptionKey == 0)
            {
                voteMode=0; 
            }

        }
        //show the client the servers response to their request
        printf("*******SERVER REPLY*******\n%s\n\n", serverReply); 

    }
    close(clientSocketfd); 
    return 0; 
}
