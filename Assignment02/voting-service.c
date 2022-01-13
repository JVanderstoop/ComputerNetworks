/**
 * 	CPSC 441, Assignment 2, Fall 2021
 *  Written by Joshua Vanderstoop 
 * 
 * Usage:
 *          gcc -o voting voting-service.c
 *          ./voting
 * program: voting-service.c
 *      - shows candidates & ID numbers upon client request
 *      - Securely processes votes from a client
 *      - produces voting results 
 * 
 * communicates with: indirection-server.c
 * acts as: Server
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

#define PORTNUM 8889

#define MAX_MESSAGE_LENGTH 1000

#define MDT (-6)

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
    char clientVote[MAX_MESSAGE_LENGTH]; 
    char serverResponse[MAX_MESSAGE_LENGTH]; 
    char clientKey[1];
    int encryptionKey, userChoice, bytes, done=0, hasVoted =0, pollsClosed = 0; 
    int KnightColby= 150, StoopSam= 180, TellyZak= 78, KnightChase= 69; 

    //setup timing system for voting to close
    time_t rawtime; 
    struct tm *currentTime; 
    int hour=0;  

    while(!done)
    {
        //every time a user joins the service, check the time to make sure the polls are still open
        time(&rawtime);
        currentTime = gmtime(&rawtime);
        hour = (currentTime->tm_hour+MDT);
        if(hour < 0 )
        {
            hour += 24; 
        }
        if(hour > 24)
        {
            hour -=24; 
        }
        printf("Current time and date(MDT): %2d:%02d on %04d/%02d/%02d\n", hour, currentTime->tm_min, 1900+(currentTime->tm_year), currentTime->tm_mon + 1, currentTime->tm_mday); 
        //modify the ability to vote based on the time being before 10pm on oct 22, 2021
        if ( hour >= 22 && 1900+(currentTime->tm_year) >=2021 && currentTime->tm_mon + 1 >=10 && currentTime->tm_mday >= 22)
        {
            pollsClosed = 1; 
            #ifdef debug
                printf("the polls are closed!\n");
            #endif
        }
        else 
        {
            pollsClosed = 0;
            #ifdef debug
                printf("the polls are open!\n");
            #endif
        }
        //clean buffers
        bzero(clientVote, MAX_MESSAGE_LENGTH); 
        bzero(serverResponse, MAX_MESSAGE_LENGTH);

        //get the conversion request
        bytes = recvfrom(serverfd, clientVote, MAX_MESSAGE_LENGTH, 0, client, &c);
        while (bytes > 0)
        {
            clientVote[bytes] = '\0';
            //begin with a handshake to send instructions
            if(strcmp(clientVote, "handshake")==0 && pollsClosed==0) //polls are open instructions
            {
                //create random encryption key from 1-9, send it to the client
                srand(time(NULL));
                encryptionKey = rand() % 10;
                if (encryptionKey ==0 )
                {
                    encryptionKey++; 
                }
                sprintf(clientKey, "%i", encryptionKey); //give client their specific key
                
                //let the client know what the protocol for use of the microservice is
                strcat(serverResponse, clientKey); // ADD CLIENT KEY
                strcat(serverResponse, "Welcome to the voting service, the polls are open!\n");
                strcat(serverResponse, "Please enter the number that corresponds to the candidate you would like to vote for.\n");
                strcat(serverResponse, "Below are the options for candidates in this voting term:\n     Colby Knight-----1\n     Zak Telly-----2\n     Sam Stoop-----3\n     Chase Knight-----4\n\n"); 
                strcat(serverResponse, "Voting will close on October 22nd, 2021 at 10:00pm MDT\n"); 
                strcat(serverResponse, "Once you have voted, you may view the polling results by reopening this service after the polls close.\n"); 
                strcat(serverResponse, "Enter QUIT to exit this service.\n"); //this will be sent to the client 

                #ifdef debug
                    printf("the current client key is: %c\n", serverResponse[0]);
                    printf("the current results are(by number of votes):\nColby Knight-----%i\nZak Telly-----%i\nSam Stoop-----%i\nChase Knight-----%i\n\n", KnightColby, TellyZak, StoopSam, KnightChase );
                #endif
            }
            else if(strcmp(clientVote, "handshake")==0 && pollsClosed==1)   //polls are closed instructions
            {
                printf("\nHELLO\n");
                //display the results, send encryption key of 0 for client
                strcat(serverResponse, "0Welcome to the voting service, the polls are closed. Please review the reuslts below.\nEnter QUIT when you are finished viewing\n");
                printf("\nHELLO\n");
                strcat(serverResponse, "Election results(by number of votes):\n"); 
                printf("\nHELLO\n");
                char buf[500];
                printf("\nHELLO\n");
                snprintf(buf, 499, "     Colby Knight-----%i\n     Zak Telly-----%i\n     Sam Stoop-----%i\n     Chase Knight-----%i\n\n", KnightColby, TellyZak, StoopSam, KnightChase );
                printf("\nHELLO\n");
                strcat(serverResponse, buf);
            }
            else if (strcmp(clientVote, "QUIT") == 0)
            {
                //quit, prepare to get other votes
                done = 1; 
                hasVoted=0; 
                strcpy(serverResponse, "Quitting the voting service\n"); 
            }
            else if ( clientVote[0] - '0' > 0 && pollsClosed==0 && hasVoted ==0)
            {
                //decrypt the users choice
                userChoice = atoi(clientVote) / encryptionKey; 

                #ifdef debug
                    printf("Using key: %i and client input: %s the locked in vote is for: %i\n", encryptionKey, clientVote, userChoice);
                #endif
                //if the user has voted, add their vote to the corresponding candidate
                if (userChoice == 1)
                {
                    KnightColby+=1; 
                    hasVoted = 1; 
                    strcpy(serverResponse, "Thank you for doing your civic duty. Please QUIT and come back later to view the results. \n");
                }
                else if (userChoice == 2)
                {
                    TellyZak+=1; 
                    hasVoted = 1; 
                    strcpy(serverResponse, "Thank you for doing your civic duty. Please QUIT and come back later to view the results. \n");
                }
                else if (userChoice == 3)
                {
                    StoopSam+=1; 
                    hasVoted = 1; 
                    strcpy(serverResponse, "Thank you for doing your civic duty. Please QUIT and come back later to view the results. \n");
                }
                else if (userChoice == 4)
                {
                    KnightChase+=1; 
                    hasVoted = 1; 
                    strcpy(serverResponse, "Thank you for doing your civic duty. Please QUIT and come back later to view the results. \n");
                }
                else 
                {   
                    //user voted for a non-existent candidate
                    hasVoted = 0; 
                    strcpy(serverResponse, "Sorry, that was not one of the possible candidates. Please try again, \n");
                }
                #ifdef debug
                    printf("the current results are(by number of votes):\nColby Knight-----%i\nZak Telly-----%i\nSam Stoop-----%i\nChase Knight-----%i\n", KnightColby, TellyZak, StoopSam, KnightChase );
                #endif
                
            }
            else if ( clientVote[0] - '0' > 0 && pollsClosed==0 && hasVoted ==1)
            {
                //user tries to vote twice in the same session
                strcpy(serverResponse, "You have already voted and cannot vote again. Please QUIT and come back later to view the results. \n");
            }
            else
            {
                //user sent some garbage the server doesnt want
                strcpy(serverResponse, "That was not a valid input. Please restart and try again.\n");
                done =1; 
            }
            #ifdef debug
                printf("Sending the response:\n\n%s\n\nto the client\n", serverResponse); 
            #endif
            //send the reply, clean the buffers and get a new request
            sendto(serverfd, serverResponse, strlen(serverResponse), 0, client, c); 
            bzero(clientVote, MAX_MESSAGE_LENGTH);
            bytes = recvfrom(serverfd, clientVote, MAX_MESSAGE_LENGTH, 0, client, &c); 
            bzero(serverResponse, MAX_MESSAGE_LENGTH); 
        }
    }
    close(serverfd); 
    return 0; 
}

        





    
