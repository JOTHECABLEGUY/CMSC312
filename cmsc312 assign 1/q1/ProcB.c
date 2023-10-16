
/*
 * shm-client - client program to demonstrate shared memory.
 * shm-client - not sure of the origin of these code segments.
 * possibly: David Marshalls course : http://www.cs.cf.ac.uk/Dave/C/CE.html
 * or Steve Holmes : http://www2.its.strath.ac.uk/courses/c/
 */
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SHMSZ     27

int
main()
{
    int int_shmid, str_shmid;
    key_t key, t;
    char *str_shm;
    int* int_shm;

    /*
     * We need to get the segment named
     * "5678", created by the server.
     */
    key = 1022;
    t = 1023;

    /*
     * Locate the segment.
     */
    if( (int_shmid = shmget(key, SHMSZ, 0744)) < 0 )
    {
        perror("shmget");
        exit(1);
    }
    if( (str_shmid = shmget(t, SHMSZ, 0744)) < 0 )
    {
        perror("shmget");
        exit(1);
    }

    /*
     * Now we attach the segment to our data space.
     */
    if( (int_shm = (int* ) shmat(int_shmid, NULL, 0)) ==  (int *) -1 )
    {
        perror("shmat");
        exit(1);
    }
    if( (str_shm = shmat(str_shmid, NULL, 0)) == (char *) -1 )
    {
        perror("shmat");
        exit(1);
    }
    
    /*
     * Now read what the server put in the memory.
     */
    
    while(*int_shm != 1)
        usleep(1);
    strcpy(str_shm, "I am Process B\n");
    *int_shm = 2;
        
    if(shmdt((void*) int_shm) == -1){
        printf("cant detach");
    }
    if(shmdt(str_shm) == -1){
        printf("cant detach");
    }
    exit(0);
    return 0;
}
