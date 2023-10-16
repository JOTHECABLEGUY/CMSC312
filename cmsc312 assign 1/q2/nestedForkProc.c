#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h> /* for exit */
#define SHMSZ 27
int main()
{
    int int_shmid, str_shmid;
    key_t key, t;
    char *str_shm;
    int* int_shm;
    int child_status;
    key = 1022;
    t = 1023;
    pid_t pid, pid2;
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
    if( (int_shm = (int*) shmat(int_shmid, NULL, 0)) == (int *) -1 )
    {
        perror("shmat");
        exit(1);
    }
    if( (str_shm = shmat(str_shmid, NULL, 0)) == (char *) -1 )
    {
        perror("shmat");
        exit(1);
    }
    
    strcpy(str_shm, "I am process A");
    printf("%s\n", str_shm);
    *int_shm = 1;
    sleep(1);
    pid = fork();
    if (pid == 0){
        while(*int_shm != 1)
            usleep(1);
        strcpy(str_shm, "I am process B");
        *int_shm = 2;
        sleep(1);
        pid2 = fork();
        if (pid2 == 0){
            usleep(2);
            while(*int_shm != 2)
                usleep(3);
            strcpy(str_shm, "I am process C");
            *int_shm = 3;
            sleep(1);
            exit(0);
        } 
        else
        {
            wait(NULL);
            exit(0);
        }
        wait(NULL);
        exit(0);
    } 
    else
    {
        while (*int_shm != 2)
            usleep(1);
        printf("%s\n", str_shm);
        while(*int_shm != 3)
            usleep(3);
        printf("%s\n", str_shm);   
    }

    waitpid(pid2, NULL, 0);

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