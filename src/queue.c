#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/queue.h"
#include "../include/myheaders.h"
#include <signal.h>


void QUEUE_init(QUEUE_PTR *head)
{
	*head=NULL;
}

void QUEUE_destroy(QUEUE_PTR *head)
{
	QUEUE_PTR ptr;
	while (*head!=NULL)
	{
		ptr = *head;
		*head=(*head)->next;
		free(ptr);
	}
}

int QUEUE_empty(QUEUE_PTR head)
{
	return head == NULL;
}

void QUEUE_print(QUEUE_PTR queue) 
{
    if (queue == NULL) {
        printf("Queue is empty.\n");
        return;
    }

    QUEUE_PTR current = queue;
    printf("Queue contents:\n");
    while (current != NULL) {
        printf("Job ID: %s, Job: %s, ACTIVE: %d  SOCKET: %d\n", current->jobId, current->job, current->active, current->socket);
        current = current->next;
    }
}

int QUEUE_count(QUEUE_PTR head)
{
	int i = 0;
	QUEUE_PTR p = NULL;
	for (p = head; p != NULL; p = p->next)
		i++;
	return i;
}

int QUEUE_countInactive(QUEUE_PTR head) {
    int count = 0;
    QUEUE_PTR current = head;

    while (current != NULL) {
        if (current->active == 0) {
            count++;
        }
        current = current->next;
    }

    return count;
}

int QUEUE_countActive(QUEUE_PTR head) {
    int count = 0;
    QUEUE_PTR current = head;

    while (current != NULL) {
        if (current->active == 1) {
            count++;
        }
        current = current->next;
    }

    return count;
}

QUEUE_PTR QUEUE_add(QUEUE_PTR head, char* JobId, char* job, int socket, int active) {
    // Allocate memory for the new node
	QUEUE_PTR new_node = (QUEUE_PTR)malloc(sizeof(QUEUE_NODE));
    if (new_node == NULL) {
        fprintf(stderr, "Memory allocation failed for new node.\n");
        exit(EXIT_FAILURE);
    }

    // Initialize the new node with the provided values
	new_node->jobId =malloc(strlen(JobId) + 1); 
	strcpy(new_node->jobId, JobId); 
    new_node->job = malloc(strlen(job) + 1); 
    strcpy(new_node->job, job);               
	new_node->socket =socket;
    new_node->active = active;
    new_node->next = NULL;

    // If the queue is empty, the new node becomes the head
    if (head == NULL) {
        return new_node;
    }

    // Otherwise, traverse the queue to find the last node
    QUEUE_PTR current = head;
    while (current->next != NULL) {
        current = current->next;
    }
    current->next = new_node;   //Added it to the end
    return head;
}

int QUEUE_deletewaiting(QUEUE_PTR *head, char *jobId)
 {
    QUEUE_PTR current = *head;
    QUEUE_PTR prev = NULL;


    while (current != NULL) {

        if (strcmp(current->jobId, jobId) == 0) {

            if (prev == NULL) { // If the node to be deleted is the first node
                *head = current->next;
            } else {
                prev->next = current->next;
            }
            free(current);
            return 1;           // Return 1 indicating deletion
        }
        prev = current;
        current = current->next;
    }
    return 0; // Return 0 indicating no deletion
}

int QUEUE_deleterunning(QUEUE_PTR *head, char *jobId)
 {
    QUEUE_PTR current = *head;
    QUEUE_PTR prev = NULL;


    while (current != NULL) {

        if (strcmp(current->jobId, jobId) == 0) {

            if (prev == NULL) { // If the node to be deleted is the first node
                *head = current->next;
            } else {
                prev->next = current->next;
            }
            free(current);
            return 1;           // Return 1 indicating deletion
        }
        prev = current;
        current = current->next;
    }
    return 0; // Return 0 indicating no deletion
}

// JOB_INFO QUEUE_getfirst(QUEUE_PTR head) {
//     if (head == NULL) {
//         printf("Queue is empty\n");
//         exit(EXIT_FAILURE);
//     }
//     JOB_INFO jobInfo;
//     jobInfo.jobId = head->jobId;
//     jobInfo.job = head->job;
//     jobInfo.queuePosition = head->queuePosition;
//     jobInfo.socket = head->socket;
//     return jobInfo;
// }

JOB_INFO QUEUE_getfirst(QUEUE_PTR head)
{
        if (head == NULL) {
        printf("Queue is empty\n");
        exit(EXIT_FAILURE);
    }

    QUEUE_PTR current = head;
    while (current != NULL) {
        if (current->active == 0) {
            JOB_INFO jobInfo;
            jobInfo.jobId = current->jobId;
            jobInfo.job = current->job;
            jobInfo.socket = current->socket;
            jobInfo.active = current->active;

            // // Set the active status to 1
            // current->active = 1;

            return jobInfo;
        }
        current = current->next;
    }

    // If no job with active == 0 is found
    printf("No job with active == 0 found\n");
    //exit(EXIT_FAILURE);
}

JOB_INFO QUEUE_getfirstwaiting(QUEUE_PTR *head) {
    if (*head == NULL) {
        printf("Queue is empty\n");
        exit(EXIT_FAILURE);
    }

    QUEUE_PTR current = *head;
    while (current != NULL) {
        if (current->active == 0) {
            JOB_INFO jobInfo;
            jobInfo.jobId = current->jobId;
            jobInfo.job = current->job;
            jobInfo.socket = current->socket;
            jobInfo.active = current->active;

            // Set the active status to 1
            current->active = 1;

            return jobInfo;
        }
        current = current->next;
    }

    // If no job with active == 0 is found
    printf("No job with active == 0 found\n");
    //exit(EXIT_FAILURE);
}

char* QUEUE_Return(QUEUE_PTR head)
{
    if (head == NULL)
    {
        char* empty_message = "Queue is empty.\n";
        char* result = malloc(strlen(empty_message) + 1);
        strcpy(result, empty_message);
        return result;
    }

    char* temp = malloc(MAXLEN * sizeof(char));
    if (temp == NULL)
    {
        printf("Error allocating memory\n");
        exit(EXIT_FAILURE);
    }
    temp[0] = '\0'; // Initialize temp to an empty string

    QUEUE_PTR current = head;

   while (current != NULL)
    {
        if (current->active == 0) {
            char* info = malloc(MAXLEN * sizeof(char));
            if (info == NULL)
            {
                printf("Error allocating memory\n");
                exit(EXIT_FAILURE);
            }

            sprintf(info, "%s, %s\n", current->jobId, current->job);
            strcat(temp, info);

            free(info); // Free memory allocated for info
        }
        current = current->next;
    }

    return temp;
}

int QUEUE_check(QUEUE_PTR head, char *jobId)
{
    QUEUE_PTR current = head;
    QUEUE_PTR prev = NULL;


    while (current != NULL) {

        if (strcmp(current->jobId, jobId) == 0) {

            if (current->active == 1) //running
            {
                return 0;
            }
            else{
                return 1;           // Return 1 indicating deletion
            }
        }
        prev = current;
        current = current->next;
    }
    return 0; // Return 0 indicating no deletion
}

JOB_INFO QUEUE_getjob(QUEUE_PTR head, const char *jobId)
{
    if (head == NULL) {
        printf("Queue is empty\n");
        exit(EXIT_FAILURE);
    }

    QUEUE_PTR current = head;
    while (current != NULL) {
        if (strcmp(current->jobId, jobId) == 0) {
            JOB_INFO jobInfo;
            jobInfo.jobId = current->jobId;
            jobInfo.job = current->job;
            jobInfo.socket = current->socket;
            jobInfo.active = current->active;

            return jobInfo;
        }
        current = current->next;
    }
}
