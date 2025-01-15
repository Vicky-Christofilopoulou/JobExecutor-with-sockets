#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "../include/queue.h"
#include "../include/myheaders.h"


//Prototypes
void jobExecutorServer(int port_num, int buffer_size, int thread_pool_size);
void * worker_thread();
void * controller_thread();


//Shared variables
int Concurrency = 1;
int buffer_size;
int id = 1;
int waiting = 0; 
int exit_flag = 0;
int thread_pool_size;
QUEUE_PTR runningQueue = NULL;     //To store the jobs that are running

pthread_t *workerthreads;

pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;    //For add and delete from queue
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;    //For the theads, to eait until something happed and no do busy waiting
pthread_cond_t full = PTHREAD_COND_INITIALIZER;

pthread_mutex_t jobID_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex for jobID generation
pthread_mutex_t waiting_mutex = PTHREAD_MUTEX_INITIALIZER;



int main(int argc, char **argv) 
{
    if (argc < 4) {
        printf("The correct format is ./jobExecutorServer [portnum] [bufferSize] [threadPoolSize].\n");
        return 0;
    }

    int port_num = atoi(argv[1]);
    buffer_size = atoi(argv[2]);
    thread_pool_size = atoi(argv[3]);
    //printf("port, %d, buf: %d, poll %d\n", port_num, buffer_size,thread_pool_size);

    if (buffer_size <= 0) {
        printf("Buffer size must be > 0\n");
        return 0;
    }

    jobExecutorServer(port_num, buffer_size, thread_pool_size);
    return 0;
}

void jobExecutorServer(int port_num, int buffer_size,int thread_pool_size)
{
    int server_sock;
    int client_sock;
    pthread_t threads[thread_pool_size];
    int connections = buffer_size;

    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    workerthreads = malloc(thread_pool_size * sizeof(pthread_t));

    //Step 1: Create the socket
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
    {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port_num);

    //Step 2: Bind socket to the port
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    //Step 3: Listen for incoming connections
    if (listen(server_sock, connections) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    //Step 4: Create worker threads
    for (int i = 0; i < thread_pool_size; i++) {
        pthread_create(&workerthreads[i], NULL, worker_thread, NULL);
    }

    //Step 5: Initialize queue
    QUEUE_init(&runningQueue);

    for (; ;)
    {
        //printf("Waiting for connections.\n");

        //Step 6: Accept 
        if ((client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len)) < 0) 
        {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }

        //Step 7: Create Controller threads
        pthread_t controllerthread; 
        int *pclient = malloc(sizeof(int));
        *pclient = client_sock;
        if (pthread_create(&controllerthread, NULL, controller_thread, pclient) != 0)
        {
            printf("Controller Thread error.\n");
        }
    }   // end for loop
    close(server_sock);  
}

void * worker_thread(void * arg)
{
    char* jobId;                  
    char * job;
    int socket;
    pid_t pid;
    char *args[MAX_ARGS];
    int i;
    char *tok;
    
    for(;;)
    {
        pthread_mutex_lock(&queue_mutex);

        if (QUEUE_empty(runningQueue) || QUEUE_countInactive(runningQueue) == 0)
        {
            // Here the worker threads wait until there is a job to execute
            printf("Sleeping:  %lu\n", pthread_self());
            pthread_cond_wait(&empty, &queue_mutex);
            printf("Woke up: %lu\n", pthread_self());
        }

        if (exit_flag == 1 )
        {
            pthread_mutex_unlock(&queue_mutex);
            printf("died %lu\n", pthread_self());
            break;
        }

        // Get the first job from the running queue
        JOB_INFO jobInfo = QUEUE_getfirstwaiting(&runningQueue);

        jobId = jobInfo.jobId;
        job = jobInfo.job;
        socket = jobInfo.socket;

        pthread_mutex_unlock(&queue_mutex);
        printf("Worker thread %ld (ID: %lu) is performing a job.\n", (long)arg, pthread_self());

        // Execute the job
        if ((pid = fork()) < 0) 
        {
            perror("fork");
            exit(1);
        }
        if (pid == 0) // Child process
        {
            // Create a file to store the output
            char output_file[20];
            sprintf(output_file, "%d.output", getpid());
            int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (fd < 0) {
                perror("Error creating output file");
                exit(EXIT_FAILURE);
            }

            // Redirect stdout to the file
            dup2(fd, STDOUT_FILENO);
            close(fd);
            
            // Parse the command string
            for (int i =0; i< MAX_ARGS; i++)
            {
                args[i] = NULL;
            }

            i = 0;
            tok = strtok(job, " ");
            while (tok != NULL) {
                args[i] = strdup(tok);
                tok = strtok(NULL, " ");
                i++;
            }
            args[i] = NULL;

            // Execute the command
            execvp(args[0], args);
            perror(args[0]);
            exit(1);
        }  
        else // Parent process
        {
            //printf("Parent process.\n");
            waitpid(pid, NULL, 0);

            // Read the output file
            char output_file[20];
            sprintf(output_file, "%d.output", pid);
            FILE *output_fp = fopen(output_file, "r");
            if (output_fp == NULL) {
                perror("Error opening output file");
                exit(EXIT_FAILURE);
            }

            // Read the contents of the file
            char output_buffer[MAX_OUTPUT_SIZE];
            fread(output_buffer, 1, MAX_OUTPUT_SIZE, output_fp);
            fclose(output_fp);

            // Send the contents back to the client
            char response[MAXLEN + MAX_OUTPUT_SIZE + 19];
            sprintf(response, "-----jobID output start-----\n%s\n-----jobID output end-----\n", output_buffer);
            write(socket, response, strlen(response));
            close(socket);
            printf("Message send.\n");

            // Clean up: remove the output file
            remove(output_file);

            if (exit_flag == 1 )
            {
                pthread_mutex_unlock(&queue_mutex);
                // printf("died %lu\n", pthread_self());
                break;
            }

            // After finishing the job, check if there are waiting jobs
            QUEUE_deleterunning(&runningQueue, jobId);

            if (QUEUE_countInactive(runningQueue) != 0)
            {
                // Get the first job from the waiting queue
                JOB_INFO waitingJobInfo = QUEUE_getfirst(runningQueue);
                jobId = waitingJobInfo.jobId;
                job = waitingJobInfo.job;
                socket = waitingJobInfo.socket;
                printf("Job added to running state: %s\n", jobId);
            }
            else if (waiting > 0)
            {
                pthread_cond_signal(&full);
            }
            pthread_mutex_unlock(&queue_mutex);
        }
    }
}

void * controller_thread(void *arg)
{
    char *instr;
    char jobpid[MAXLEN];
    char response[MAXLEN + 19];
    char *tok;

    int client_sock = *((int *)arg);
    free(arg);          // Free the memory allocated for the argument

    // Buffer to store the command received from the client
    char readbuf[MAXLEN];
    memset(readbuf, 0, sizeof(readbuf));

    // Read the command from the client
    read(client_sock, readbuf, sizeof(readbuf));
    printf("readbuf: %s\n", readbuf);
    
    instr = strtok(readbuf, " ");

    //Based on the instruction i do what i have to do
    if (strcmp(instr, "issueJob") == 0)
    {
        
        pthread_mutex_lock(&queue_mutex);
        
        //Check if the queue has space
        if (QUEUE_count(runningQueue) >= buffer_size)
        {
            // Here the worker threads wait until there is a job to execute
            pthread_mutex_lock(&waiting_mutex);
            waiting ++;
            tok = strtok(NULL, "\n");
            pthread_mutex_unlock(&waiting_mutex);
            pthread_cond_wait(&full, &queue_mutex);

            if (exit_flag == 1)
            {
                sprintf(response, "SERVER TERMINATED BEFORE EXECUTION.\n");
                write(client_sock, response, strlen(response));
                close(client_sock);
            }
        }
    
        if (waiting == 0)
        {
            tok = strtok(NULL, "\n");
        }
        else
        {
            pthread_mutex_lock(&waiting_mutex);
            waiting--;
            pthread_mutex_unlock(&waiting_mutex);
        }
        pthread_mutex_lock(&jobID_mutex);
        sprintf(jobpid, "job_%d", id);
        id++;
        pthread_mutex_unlock(&jobID_mutex);
        
        JOB_INFO jobInfo = { .jobId = strdup(jobpid), .job = strdup(tok), .socket = client_sock };

        if (QUEUE_count(runningQueue) < Concurrency && Concurrency <= buffer_size )
        {
            runningQueue = QUEUE_add(runningQueue, jobpid, tok, client_sock, 0); // Update running queue
            printf("Job %s added to running state.\n", jobpid);
            pthread_cond_signal(&empty);
        }
        else if (Concurrency <= buffer_size) //palia waiting
        {
            runningQueue = QUEUE_add(runningQueue, jobpid, tok, client_sock,0); // Update waiting queue
            printf("Job %s added to waiting state.\n", jobpid);
        }
        pthread_mutex_unlock(&queue_mutex);
        sprintf(response, "JOB <%s, %s> SUBMITTED", jobpid, tok);
        write(client_sock, response, strlen(response));

    }
    else if(strcmp(instr, "setConcurrency") == 0)
    {
        int ConcFlag = 0;                           //ConcFlag is 0 when concurrency was not increased
        char * con = strtok(NULL, " ");
        if (atoi(con) > buffer_size)
        {
            sprintf(response, "%s %d","Could not change concurrency, beacuse it is less than the buffer size. Concurrency:", Concurrency);
            write(client_sock,response,strlen(response));
        }
        else if (atoi(con) > thread_pool_size)
        {
            sprintf(response, "%s %d","Could not change concurrency, beacuse it is less than the thread pool size. Concurrency:", Concurrency);
            write(client_sock,response,strlen(response));
        }
        else if (atoi(con) > Concurrency )                //if Concurrency was increased raise ConcFlag
        {
            ConcFlag = 1;
            Concurrency = atoi(con);
        }               
        
        //Now i have to check if i need to add or remove jobs
        if (ConcFlag == 1)
        {
            Update();sprintf(response, "%s %d", "CONCURRENCY SET AT:", Concurrency);
            write(client_sock,response,strlen(response));
        }

    }
    else if(strcmp(instr, "stop") == 0)
    {
        
        char * tok = strtok(NULL, " ");
        int check = QUEUE_check(runningQueue, tok);
        if (check == 0 ) //Running or not found
        {
            sprintf(response, " JOB <%s> NOT FOUND.", tok);
            write(client_sock,response,strlen(response));
        }
        else    //waiting
        {
            JOB_INFO jobInfo = QUEUE_getjob(runningQueue, tok);
            char * jobId = jobInfo.jobId;
            char * job = jobInfo.job;
            int sok = jobInfo.socket;
            
            pthread_mutex_lock(&queue_mutex);
            QUEUE_deleterunning(&runningQueue, tok);
            pthread_mutex_unlock(&queue_mutex);

            sprintf(response, " JOB <%s> REMOVED.", tok);
            write(client_sock,response,strlen(response));

            //Issue job waits for 2 replys so send and newline
            sprintf(response, "This job was removed.\n");
            write(sok,response,strlen(response));
            close(sok); 
            Update();
        }
    }
    else if(strcmp(instr, "poll") == 0)
    {
        char *results;
        pthread_mutex_lock(&queue_mutex);                      
        results = QUEUE_Return(runningQueue);
        pthread_mutex_unlock(&queue_mutex);
        write(client_sock,results,strlen(results));
    }
    else  //We know it is exit because we have validate the form of instruction from main
    {
        exit_flag = 1;
        Concurrency = 0;

        // Empty waiting queue and notify clients
        pthread_mutex_lock(&queue_mutex);
        while (QUEUE_countInactive(runningQueue) > 0)
        {
            JOB_INFO waitingJobInfo = QUEUE_getfirst(runningQueue);
            int clientSocket = waitingJobInfo.socket;
            sprintf(response, "SERVER TERMINATED BEFORE EXECUTION.\n");
            write(clientSocket, response, strlen(response));
            close(clientSocket);
            QUEUE_deletewaiting(&runningQueue, waitingJobInfo.jobId);
        }
        pthread_mutex_unlock(&queue_mutex);

        pthread_cond_broadcast(&empty);
        pthread_cond_broadcast(&full);

        // Wait for all worker threads to finish
        for(int i=0; i < thread_pool_size; i++) {
            pthread_join(workerthreads[i], NULL);
        }

        sprintf(response, "%s", "jobExecutorServer terminated");
        write(client_sock,response,strlen(response));
        close(client_sock);
        exit(0);
    }
}

// -------------------------------FUNCTIONS---------------------------------------------
void Update() 
{
    int running = QUEUE_countActive(runningQueue);
    QUEUE_PTR temp;
    char *jobId;
    char *job;
    int sok;

    pthread_mutex_lock(&queue_mutex);
    if (running < Concurrency && Concurrency <= buffer_size) 
    { // Check if running jobs are less than Concurrency
        while (running < Concurrency && Concurrency <= buffer_size) 
        {             
            if(QUEUE_count(runningQueue) == 0 || QUEUE_countInactive (runningQueue) == 0)
            {
                break;
            }

            JOB_INFO jobInfo = QUEUE_getfirst(runningQueue);

            jobId = jobInfo.jobId;
            job = jobInfo.job;
            sok = jobInfo.socket;
        
            printf("Job added to running state: %s\n", jobId);
            pthread_cond_signal(&empty);
            running = QUEUE_countActive(runningQueue) +1 ;//Count the one i also added now
        }
    }
    pthread_mutex_unlock(&queue_mutex);
    printf("Update end.\n");
}