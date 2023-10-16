//Jordan Dube
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#define BILLION  1000000000.0; 
//min function that returns min of 2 numbers
int min(int a, int b){
    return(a > b) ? b : a;
}
//stub for program exit
void termHandler();
//added csem
struct CSEM {
    int val;
    sem_t gate;
    sem_t mutex;
}; 
//printerjob struct
struct printJob {
    pid_t pid;
    int numBytes;
    struct timespec creationTime;
};
//implementation of correct CSEM struct methods
void Pc(struct CSEM *cs){
    sem_wait(&(cs->gate));
    sem_wait(&(cs->mutex));
    cs->val--;
    if(cs->val>0){
        sem_post(&(cs->gate));
    }
    sem_post(&(cs->mutex));
}
void Vc(struct CSEM *cs){
    sem_wait(&(cs->mutex));
    cs->val++;
    if(cs->val == 1){
        sem_post(&(cs->gate));
    }
    sem_post(&(cs->mutex));
}
void Dc(struct CSEM *cs){
    sem_destroy(&(cs->gate));
    sem_destroy(&(cs->mutex));
}
void Ic(struct CSEM *cs, int _pthreadShared, int value){
    cs->val = value;
    sem_init(&(cs->gate), _pthreadShared, min(1, cs->val));
    sem_init(&(cs->mutex), _pthreadShared, 1);
}
//typedef to make it easier to write
typedef struct CSEM cs;
typedef struct printJob pj;
//pointer to printQueue
pj* printQueue;
//pointer to counting semaphore array
cs* countingSemArray;
//declaration of all SHM ids as well as misc. int variables
int countingSemArraySHMID, pqSHMID, binSemID, InSHMID, OutSHMID, numProdSHMID, prodSumSHMID, 
    pidsSHMID, pidSizeSHMID, totalWaitTimeSHMID, num_of_producers, num_of_consumers, ppid, conSum, numCons, i;
//pointer to binary semaphore
pthread_mutex_t* binSem;
//key for the print queue
key_t printQueueKey = 1131;
//variables to be put in shared memory
int *In, *Out, *numProd, *prodSum, *pidSize, *pids;
//variables to determine waiting and execution times
struct timespec start_time, end_time, job_end_time;
double timeSpent;
double* totalWaitTime, job_wait_time;
/* initially buffer will be empty.  full_sem
   will be initialized to buffer SIZE, which means
   SIZE number of producer threads can write to it.
   And empty_sem will be initialized to 0, so no
   consumer can read from buffer until a producer
   thread posts to empty_sem */
cs full_sem;  /* when 0, buffer is full */
cs empty_sem; /* when 0, buffer is empty. Kind of
                    like an index for the buffer */
//insert function that uses the index stored in In to place the given job into the queue
void insertbuffer(pj job) {
    //checks if there are still empty slots
    if ((*In) < 30) {
        //inserts the given job
        printQueue[(*In)] = job;
        //increments In
        *In = ((*In) + 1) % 30;
    } 
    else {
        printf("Buffer overflow\n");
    }
}
//dequeue function that returns the job at the current *In index, else returns an empty job
pj dequeuebuffer() {
    pj job;
    job.numBytes = 0;
    //Checks if the in pointer is 0 or above, meaning that there is something in the queue
    if ((*In) >= 0) {
        //increases out
        *Out = ((*Out) + 1) % 30;
        //returns the next element in the queue, still have to dereference the pointer
        return printQueue[(*Out)]; // buffer_index-- would be error!
    } else {
        printf("Buffer underflow\n");
    }
    return job;
}
//producer function
void *producer() {
    //new pj object
    pj job;
    //set the pid field of the new job
    job.pid = getpid();
    //set the creation time of the job being produced to the current time
    clock_gettime(CLOCK_REALTIME, &job.creationTime);
    i=0;
    //reseed the random generator
    srand(time(NULL) ^ (getpid()<<16));
    //set the number of loops for the current process to a random number between 1 and 30
    int loops = (rand()%30 + 1);
    //each producer iterates for the specified number of loops
    while (i++ < loops) {
        //reseed random
        srand(time(NULL) ^ (getpid()<<16));
        //sleep for a random amount of microseconds (between .1 and 1 second)
        usleep( rand()%900001 + 100000);
        //reseed random
        srand(time(NULL) ^ (getpid()<<16));
        //set number of bytes taken by the print job to a random number between 100 and 1000
        job.numBytes = (rand() % 901) + 100;
        Pc(&countingSemArray[0]); // sem=0: wait. sem>0: go and decrement it
        /* possible race condition here. After this thread wakes up,
           another thread could aqcuire mutex before this one, and add to list.
           Then the list would be full again
           and when this thread tried to insert to buffer there would be
           a buffer overflow error */
        pthread_mutex_lock(binSem); /* protecting critical section */
        //insert the job with now complete pid and number of bytes fields
        insertbuffer(job);
        //these need to be in the critical section to avoid race conditions
        //increase the number of jobs produced
        (*numProd)++;
        //increases the size of jobs produced
        (*prodSum) += job.numBytes;
        pthread_mutex_unlock(binSem);
        Vc(&countingSemArray[1]); // post (increment) emptybuffer semaphore
        printf("Producer %d added %d to buffer\n", job.pid, job.numBytes);
        
    }
    //decrease number of open processes
    (*pidSize)--;
    exit(0);
}
//consumer function
void *consumer(void *thread_n) {
    //sets id of the thread to the number that was passed by main
    int thread_numb = *(int *)thread_n;
    //printer job to hold result of dequeueing
    pj job;
    //infinite loop
    while (1) {
        //sleep to let producers add to queue first
        usleep(100000);
        //if the queue is empty AND there are no open processes, then calls termHandler to terminate the program (NORMAL termination)
        if(countingSemArray[1].val==0 && (*pidSize)==0){
                termHandler();
        }
        Pc(&countingSemArray[1]);
        /* there could be race condition here, that could cause
           buffer underflow error */
        pthread_mutex_lock(binSem);
        //store dequeued job
        job = dequeuebuffer(job);
        //increase number of bytes consumed
        conSum += job.numBytes;
        //get the end time of the job being consumed
        clock_gettime(CLOCK_REALTIME, &job_end_time);
        //getting wait time of the current job
        job_wait_time =
            ( job_end_time.tv_sec - job.creationTime.tv_sec )
          + ( job_end_time.tv_nsec - job.creationTime.tv_nsec )
            / BILLION;
        //add the wait time to the total wait time of all jobs
        (*totalWaitTime) += job_wait_time;
        pthread_mutex_unlock(binSem);
        Vc(&countingSemArray[0]); // post (increment) fullbuffer semaphore
        printf("Consumer %d dequeue %d, %d bytes from buffer\n", thread_numb, job.pid, job.numBytes);
        //increment number of jobs consumed
        numCons++; 
   }
}
//pointer to thread array to be populated in main
pthread_t *thread;
//helper method to detach and clear shared memory
void detachMemory(){
    pthread_mutex_destroy(binSem);
    Dc(&countingSemArray[0]);
    Dc(&countingSemArray[1]);
    shmdt(binSem);
    shmdt(countingSemArray);
    shmdt(printQueue);
    shmdt(In);
    shmdt(Out);
    shmdt(numProd);
    shmdt(prodSum);
    shmdt(pids);
    shmdt(pidSize);
    shmdt(totalWaitTime);
    shmctl(totalWaitTimeSHMID, IPC_RMID, NULL);
    shmctl(pidSizeSHMID, IPC_RMID, NULL);
    shmctl(pidsSHMID, IPC_RMID, NULL);
    shmctl(prodSumSHMID, IPC_RMID, NULL);
    shmctl(numProdSHMID, IPC_RMID, NULL);
    shmctl(InSHMID, IPC_RMID, NULL);
    shmctl(OutSHMID, IPC_RMID, NULL);
    shmctl(binSemID, IPC_RMID, NULL);
    shmctl(countingSemArraySHMID, IPC_RMID, NULL);
    shmctl(pqSHMID, IPC_RMID, NULL);    
}
//signal handler for control+c (SIGINT)
void controlHandler(){
    //only executes for 1 time, in parent process
    if(getpid() == ppid){
        //lock the mutex so that no producers/consumers print while threads and processes are terminated
        pthread_mutex_lock(binSem);
        //cancel the threads
        for(i = 0; i < num_of_consumers; i++)
            pthread_cancel(thread[i]);
        //kill the open processes
        for(i = 0; i < num_of_producers; i++)
            kill(pids[i], SIGKILL);
        //detach and clear the shared memory
        detachMemory();
        //exit the program
        exit(0);
    }
}
//handler method for normal program termination
void termHandler(){
    if(getpid() == ppid){
        //print the bookkeeping information
        printf("Number of Jobs Produced: %d\nNumber of Bytes Produced: %d\n", (*numProd), (*prodSum));
        printf("Number of Jobs Ponsumed: %d\nNumber of Bytes Consumed: %d\n", numCons, conSum);
        printf("Average Wait Time: %.9f\n", (*totalWaitTime)/(*numProd));
        //detach and clear the shared memory
        detachMemory();
        //store the end_time
        clock_gettime(CLOCK_REALTIME, &end_time);
        //calculate the total time taken by the program
        timeSpent = ( end_time.tv_sec - start_time.tv_sec )
          + ( end_time.tv_nsec - start_time.tv_nsec )
            / BILLION;
        printf("Execution Time: %.9f\n", timeSpent);
        //exit the program
        exit(0);
    }
}
int main(int argc, char *argv[]) {
    //initialize time spent in program
    timeSpent= 0.0;
    //get start time for program
    clock_gettime(CLOCK_REALTIME, &start_time);
    //set ppid field
    ppid = getpid();
    //signal handler is always active
    signal(SIGINT, controlHandler);
    //if the wrong number of arguments are given, exit 
    if(argc != 3){
        exit(0);
    }
    //set number of producers and consumers using the command line
    num_of_producers = atoi(argv[1]);
    num_of_consumers = atoi(argv[2]);
    //put the counting semaphores into shared memory
    countingSemArraySHMID = shmget(IPC_PRIVATE, sizeof(struct CSEM)*2, IPC_CREAT | 0666);
    countingSemArray = (cs*) shmat(countingSemArraySHMID, NULL, 0);
    //put the printQueue into shared memory
    pqSHMID = shmget(printQueueKey, sizeof(pj)*30, 0666 | IPC_CREAT);
    printQueue = shmat(pqSHMID, NULL, 0);
    //put the binary semaphore into shared memory
    binSemID = shmget(IPC_PRIVATE, sizeof(pthread_mutex_t), IPC_CREAT | 0666);
    binSem = shmat(binSemID, NULL, 0);
    //put the In index into shared memory
    InSHMID = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    In = (int *) shmat(InSHMID, NULL, 0);
    //put the Out index into shared memory
    OutSHMID = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    Out = (int *) shmat(OutSHMID, NULL, 0);
    //put the number of jobs produced into shared memory
    numProdSHMID = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    numProd = (int *) shmat(numProdSHMID, NULL, 0);
    //put the number of bytes produced into shared memory
    prodSumSHMID = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    prodSum = (int *) shmat(prodSumSHMID, NULL, 0);
    //put the open processes into shared memory
    pidsSHMID = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    pids = (int *) shmat(pidsSHMID, NULL, 0);
    //put the number of open processes into shared memory
    pidSizeSHMID = shmget(IPC_PRIVATE, sizeof(int)*num_of_producers, IPC_CREAT | 0666);
    pidSize = (int *) shmat(pidSizeSHMID, NULL, 0);
    //add totalWaitTime into shared memory
    totalWaitTimeSHMID = shmget(IPC_PRIVATE, sizeof(double), IPC_CREAT | 0666);
    totalWaitTime = (double *) shmat(totalWaitTimeSHMID, NULL, 0);
    //set the accumulation variables at 0
    *totalWaitTime = 0.0;
    *prodSum = 0;
    conSum = 0;
    //sets the in and out pointers to 0 and -1 respectively
    *In = 0;
    *Out = -1;
    //initialize the semaphores
    pthread_mutex_init(binSem, NULL);
    Ic(&full_sem, 1, 30); //full sem is 30 since the max size of the queue is 30
    Ic(&empty_sem, 1, 0);
    //put the initialized semaphores into shared memory
    countingSemArray[0] = full_sem;
    countingSemArray[1] = empty_sem;
    /* full_sem is initialized to buffer size because SIZE number of
       producers can add one element to buffer each. They will wait
       semaphore each time, which will decrement semaphore value.
       empty_sem is initialized to 0, because buffer starts empty and
       consumer cannot take any element from it. They will have to wait
       until producer posts to that semaphore (increments semaphore
       value) */
    
    //set an array to hold the consumer threads
    pthread_t consumers[num_of_consumers];
    //set the pointer declared above the detach memory method to let the handler access the consumers
    thread = consumers;
    //array of thread IDs
    int thread_numb[num_of_consumers];
    //create n number of consumers based on user input
    for(i = 0; i < num_of_consumers; i++){
        //ID = i
        thread_numb[i] = i;
        pthread_create(thread + i, // pthread_t *t
                       NULL, // const pthread_attr_t *attr
                       consumer, // void *(*start_routine) (void *)
                       thread_numb + i);  // void *arg
    }
    //fork n number of times based on provided input
    for(i = 0; i < num_of_producers; i++) 
    {
        //for each child, call producer method
        if(fork() == 0)
        {
            //add new entry in open processes array
            pids[i] = getpid();
            //increase pidSize
            (*pidSize)++;
            producer();
            exit(0);
        }
    }
    //let the consumers finish
    for(i = 0; i < num_of_consumers; i++){
        pthread_join(thread[i], NULL);
    }
    return 0;
}