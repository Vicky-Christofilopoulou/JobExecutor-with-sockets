struct node
{
	char * jobId;                  // The 3 characteristics that we get from issueJob
	char * job;
	int socket;
    int active;                  //0 not running - 1 running
	struct node * next;          //next in the queque
};

// Struct for job information - For QUEUE_Return function
struct JobInfo {
    char *jobId;
    char *job;
    int active; 
    int socket;
};

typedef struct JobInfo JOB_INFO;
typedef struct node QUEUE_NODE;
typedef struct node *QUEUE_PTR;

extern QUEUE_PTR runningQueue; // To store the jobs that are running

void QUEUE_init(QUEUE_PTR *head);       
void QUEUE_destroy(QUEUE_PTR *head);
int QUEUE_empty(QUEUE_PTR head);
void QUEUE_print(QUEUE_PTR queue);
int QUEUE_count (QUEUE_PTR head);
int QUEUE_countActive(QUEUE_PTR head);
int QUEUE_countInactive(QUEUE_PTR head) ;
QUEUE_PTR QUEUE_add(QUEUE_PTR head, char* JobId, char* job, int socket, int active) ;
int QUEUE_deletewaiting(QUEUE_PTR *head, char *jobId);
int QUEUE_deleterunning(QUEUE_PTR *head, char *jobId);
JOB_INFO QUEUE_getfirst(QUEUE_PTR head);
JOB_INFO QUEUE_getfirstwaiting(QUEUE_PTR *head);
char *QUEUE_Return(QUEUE_PTR head);
int QUEUE_check(QUEUE_PTR head, char *jobId);
JOB_INFO QUEUE_getjob(QUEUE_PTR head, const char *jobId);