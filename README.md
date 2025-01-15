# JobExecutor-with-sockets

## 1. Architecture and Processes

The application consists of two main components:

### jobCommander:
The jobCommander is responsible for managing commands issued by the user and sending them to the server via the network.  
This can be a client application that connects to the server, sends job requests, and handles responses from the server.

### jobExecutorServer:
The server listens for incoming client connections, processes commands, and manages tasks.  
It uses threads to manage task execution and concurrency.

#### Main Components of the Server:
- **Main thread**:  
  Manages client connections and delegates tasks to the controller threads.

- **Controller threads**:  
  Handle the commands coming from the jobCommander. These threads may add jobs to a job queue (buffer) for processing.

- **Worker threads**:  
  Execute the tasks. Each worker thread processes a task when it becomes available.

---

## 2. Core Functions

- **issueJob**: Adds a new job to the buffer and returns a jobID.  
- **setConcurrency**: Sets the level of concurrency (i.e., the number of simultaneous worker threads).  
- **stop**: Removes a job from the buffer, if present.  
- **poll**: Returns a list of all waiting jobs in the buffer.  
- **exit**: Terminates the server after completing the tasks.

---

## 3. Managing Concurrency and Buffer

The buffer holds jobs that are waiting to be executed by the worker threads.  
The worker threads should wait for new jobs if none are available.  
When a worker thread becomes available, it will take over the execution of a job using `fork()` and `exec()` to run the job in a new child process.

---

## 4. Using Condition Variables

Condition variables must be used for synchronization between threads. This is necessary to avoid busy-waiting situations and ensure that threads wait and work in sequence without consuming unnecessary CPU time.

---

## 5. Code Organization and Submission

The code should be well-organized with at least two files, using the method of separate compilation.  
The code should contain sufficient comments and follow good software engineering practices.
