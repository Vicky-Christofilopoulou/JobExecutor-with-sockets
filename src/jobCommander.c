#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "../include/myheaders.h"


void *handle_issue_job(void *arg) {
    int sock = *((int *)arg);
    char response[MAXLEN];
    memset(response, 0, sizeof(response));

    // Read the initial response from the server
    read(sock, response, MAXLEN);
    printf("Response from server: %s\n", response);
    memset(response, 0, sizeof(response));

    // Continue reading the output from the server
    int valread;
    while ((valread = read(sock, response, sizeof(response) - 1)) > 0) {
        printf("%s", response);
        memset(response, 0, sizeof(response)); // Clear buffer for next read
    }

    // Check for read errors
    if (valread < 0) {
        perror("read");
    }

    // Close the socket when done
    close(sock);
    return NULL;
}

int main(int argc, char **argv) 
{

    //Check the argyments that i got from the user to define the action
    //printf("Argument : %s\n", argv[3]);

    if (argc < 4)
	{
        printf("The correct format is ./jobCommander [serverName] [portNum] [jobCommanderInputCommand].\n");
        return 0;        
    }
    // We have this form jobCommander ..
    char *server_name = argv[1];
    int port_num = atoi(argv[2]);

    if (strcmp (argv[3], "issueJob") == 0)
    {
        if (argc == 4)
        {
            printf("Wrong input of argyments for intrunction issueJob\n.");
            return 1; 
        }
    }
    else if (strcmp (argv[3], "setConcurrency") == 0)
    {
        if (argc == 4)
        {
            printf("Wrong input of argyments for intrunction setConcurrency\n.");
            return 1; 
        }
        else if (argc > 5)
        {
            printf("Too many argyments for intrunction setConcurrency. We need only 5\n.");
            return 1; 
        }
        else    // Check the concurrency is valid
        {
            int con = atoi(argv[4]);
            if (con < 1)
            {
                printf("Not acceptable concurrency. Must be higher that 1\n.");
                return 1;
            }
        }
    }
    else if (strcmp (argv[3], "stop") == 0)
    {
        if (argc == 4)
        {
            printf("Wrong input of argyments for intrunction stop.\n");
            return 1; 
        }
        else if (argc > 5)
        {
            printf("Too many argyments for intrunction stop. We need only 2.\n");
            return 1; 
        }
    }
    else if (strcmp (argv[3], "poll") == 0)
    {
        if (argc > 5)
        {
            printf("Wrong input of argyments for intrunction poll.\n");
            return 1; 
        }
    }
    else if (strcmp (argv[3], "exit") == 0)
    {
        if (argc > 5)
        {
            printf("Wrong input of argyments for intrunction exit.\n");
            return 1; 
        }
    }
    else
    {
        printf("You have not entered any valid command (issueJob,setConcurrency, stop, poll, exit).\n");
        return 0;
    }
    
    // Here we have a valid form of our instruction so it is time for the commander.
    int sock;
    struct sockaddr_in server_addr;
    struct hostent* host;
    
    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    //Prepare the sockaddr_in structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_num);

    //Resolve the server name to an IP address
    host = gethostbyname(server_name);
    if(host == NULL){
        perror("Cannot resolve hostname\n");
        exit(EXIT_FAILURE);
    }
    
    //Convert the first IP address to a string
    char* resolved_ip = inet_ntoa(*(struct in_addr*) host->h_addr_list[0]);

     //Convert the IPv4 addresses from text to binary form
    if(inet_pton(AF_INET, resolved_ip, &server_addr.sin_addr)<= 0){
        perror("Invalid address\n");
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    //Make all the argyments a string
    char writebuf[MAXLEN];              //Keep the arguments passed in order to write them in pipe
	writebuf[0] = '\0';                 //Empty string

    int i =0;
    for (i = 3; i < argc; i++)
	{
		strcat(writebuf, argv[i]);      //Ignore the first arg (programm name) and concatenate everything else
		strcat(writebuf, " ");
	}

	// printf("writebuf: %s\n", writebuf);

    // Send the command to the server
    send(sock, writebuf, strlen(writebuf), 0);

    // If the command is "issueJob", handle its responses in a separate thread
    if (strcmp(argv[3], "issueJob") == 0) {
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handle_issue_job, (void *)&sock);
        pthread_join(thread_id, NULL); // Wait for the thread to finish
    } else {
        // For other commands, read and print the server response
        char response[MAXLEN];
        int valread = read(sock, response, sizeof(response) - 1);
        if (valread > 0) {
            response[valread] = '\0';
            printf("Response from server: %s\n", response);
        }
        // Close the socket for other commands
        close(sock);
    }
}