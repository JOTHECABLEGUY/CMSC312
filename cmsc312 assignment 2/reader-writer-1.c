#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
/*
This program provides a possible solution for first readers writers problem using 
mutex and semaphore.
I have used 10 readers and 5 producers to demonstrate the solution. You can always 
play with these values.
*/
int min(int a, int b){
    return(a > b) ? b : a;
}
struct CSEM {
    int val;
    sem_t gate;
    sem_t mutex;
}; 
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
int return_val(struct CSEM *cs){
    return cs->val;
}
struct CSEM wrt;
int n = 3;
pthread_mutex_t mutex;
int cnt = 1;
int numreader = 0;
void *writer(void *wno)
{   
    Pc(&wrt);
    cnt = cnt*2;
    printf("Writer %d modified cnt to %d\n",(*((int *)wno)),cnt);
    Vc(&wrt);
}
void *reader(void *rno)
{   
    // Reader acquire the lock before modifying numreader
    pthread_mutex_lock(&mutex);
    numreader++;
    if(numreader == 1) {
        Pc(&wrt); // If this id the first reader, then it will block the writer
    }
    pthread_mutex_unlock(&mutex);
    // Reading Section
    Pc(&wrt);
    printf("Reader %d: read cnt as %d\n",*((int *)rno),cnt);
    printf("num readers: %d\n", n-return_val(&wrt));
    Vc(&wrt);
    // Reader acquire the lock before modifying numreader
    pthread_mutex_lock(&mutex);
    numreader--;
    if(numreader == 0) {
        Vc(&wrt); // If this is the last reader, it will wake up the writer.
    }
    pthread_mutex_unlock(&mutex);
}
int main()
{   
    pthread_t read[10],write[5];
    pthread_mutex_init(&mutex, NULL);
    Ic(&wrt,0,n);
    int a[10] = {1,2,3,4,5,6,7,8,9,10}; //Just used for numbering the producer and consumer
    for(int i = 0; i < 10; i++) {
        pthread_create(&read[i], NULL, (void *)reader, (void *)&a[i]);
    }
    for(int i = 0; i < 5; i++) {
        pthread_create(&write[i], NULL, (void *)writer, (void *)&a[i]);
    }
    for(int i = 0; i < 10; i++) {
        pthread_join(read[i], NULL);
    }
    for(int i = 0; i < 5; i++) {
        pthread_join(write[i], NULL);
    }
    pthread_mutex_destroy(&mutex);
    Dc(&wrt);
    return 0;
    
}