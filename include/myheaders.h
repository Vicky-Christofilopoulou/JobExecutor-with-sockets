#define MAX_ARGS 8
#define MAXLEN 700          // For buffers
#define MAX_OUTPUT_SIZE 1000

extern int Concurrency;
extern int buffer_size;

void jobExecutorServer(int port_num, int buffer_size, int thread_pool_size);
void * worker_thread();
void * controller_thread();
void Update();
