#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h> /* for exit */
#include <errno.h>


/* 
 * shm-server - not sure of the origin of code segment.
 * Old source, not sure of the origin
 *  possibly: David Marshalls course : http://www.cs.cf.ac.uk/Dave/C/CE.html
 * or Steve Holmes : http://www2.its.strath.ac.uk/courses/c/
 */



#define SHMSZ     27


int 
main()
{
    int int_shmid, str_shmid;
    key_t key, t;
    char *str_shm;
    int* int_shm;

    /*
     * We'll name our shared memory segment
     * "5678".
     */
    key = 1022;
    t = 1023;

    /*
     * Create the segment.
     */
    if( (int_shmid = shmget(key, SHMSZ, IPC_CREAT | 0744)) < 0 )
    {
        perror("shmget");
        exit(1);
    }
    
    if( (str_shmid = shmget(t, SHMSZ, IPC_CREAT | 0744)) < 0 )
    {
        perror("shmget");
        exit(1);
    }
    
    /*
     * Now we attach the segment to our data space.
     */
    if( ( int_shm = (int *) shmat(int_shmid, NULL, 0)) ==  (int *) -1 )
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
     * Now put some things into the memory for the
     * other process to read.
     */
    strcpy(str_shm, "I am Process A\n");
    printf("%s", str_shm);
    *int_shm = 1;

    /*
     * Finally, we wait until the other process 
     * changes the first character of our memory
     * to '*', indicating that it has read what 
     * we put there.
     */
    
    while(*int_shm != 2)
        usleep(1);
    printf("%s", str_shm);
    sleep(2);
    while(*int_shm != 3)
        usleep(2);
    printf("%s", str_shm);
    
    if(shmdt(int_shm) == -1){
        printf("cant detach");
    }
    if(shmdt(str_shm) == -1){
        printf("cant detach");
    }
    printf("Goodbye\n");
    shmctl(int_shmid, IPC_RMID, NULL);
    shmctl(str_shmid, IPC_RMID, NULL);
    return 0;
}
