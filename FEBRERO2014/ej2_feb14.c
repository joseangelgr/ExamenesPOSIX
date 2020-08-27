//
// Created by Jose on 27/08/2020.
//

#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <time.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define MIN 0
#define MAX 30

typedef struct {
    pthread_t thread;
    int temperatura;
    pthread_mutex_t mutex;
    int periodo;
}Deposito;

typedef struct {
    Deposito dep1,dep2;
    pthread_t monitor;
    pthread_mutex_t random;
    int periodo;
}Control;

static inline int generateRandom(pthread_mutex_t* mutex){
    int r;
    pthread_mutex_lock(mutex);
    r = MIN + random() % (MAX-MIN+1);
    pthread_mutex_unlock(mutex);
    return r;
}

static int createMutex(pthread_mutex_t* mutex){
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setprotocol(&attr,PTHREAD_PRIO_INHERIT);
    int r = pthread_mutex_init(mutex,&attr);
    pthread_mutexattr_destroy(&attr);
    return r;
}

static void createThread(pthread_t* thread, void* arg, int prio, void* (func)()){
    struct sched_param param = {prio};
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setinheritsched(&attr,PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr,SCHED_FIFO);
    pthread_attr_setschedparam(&attr, &param);

    pthread_create(thread,&attr,func,arg);
    pthread_attr_destroy(&attr);

}

static int changePriority(int sched,int prio){
    struct sched_param param = {prio};
    return pthread_setschedparam(pthread_self(),sched,&param);
}

void* deposito1(void* arg){
    Control* c = (Control*) arg;
    struct timespec ispec;
    clock_gettime(CLOCK_MONOTONIC,&ispec);
    ispec.tv_sec += c->dep1.periodo;

    while(true){
        while(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ispec, NULL) == EINTR);
        pthread_mutex_lock(&c->dep1.mutex);
        c->dep1.temperatura = generateRandom(&c->random);
        if(c->dep1.temperatura < 0.8 * MAX ){
            printf("[D1] Temperatura optima\n");
        }
        else if(c->dep1.temperatura <= 0.9 * MAX){
            printf("[D1] Temperatura Alta\n");
        }

        else{
            printf("[D1] Temperatura extrema\n");
        }

        pthread_mutex_unlock(&c->dep1.mutex);
        ispec.tv_sec += c->dep1.periodo;
    }
}

void* deposito2(void* arg){
    Control* c = (Control*) arg;
    struct timespec ispec;
    clock_gettime(CLOCK_MONOTONIC,&ispec);
    ispec.tv_sec += c->dep2.periodo;

    while(true){
        while(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ispec, NULL) == EINTR);
        pthread_mutex_lock(&c->dep2.mutex);
        c->dep2.temperatura = generateRandom(&c->random);
        if(c->dep2.temperatura < 0.8 * MAX ){
            printf("[D2] Temperatura optima\n");
        }
        else if(c->dep2.temperatura <= 0.9 * MAX){
            printf("[D2] Temperatura Alta\n");
        }

        else{
            printf("[D2] Temperatura extrema\n");
        }

        pthread_mutex_unlock(&c->dep2.mutex);
        ispec.tv_sec += c->dep2.periodo;
    }
}

void* monitor(void* arg){
    Control* c = (Control*) arg;
    struct timespec ispec;
    clock_gettime(CLOCK_MONOTONIC,&ispec);
    ispec.tv_sec += c->periodo;

    while(true){
        while(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ispec, NULL) == EINTR);

        pthread_mutex_lock(&c->dep1.mutex);
        printf("[D1] TEMPERATURA = %d \n",c->dep1.temperatura);
        pthread_mutex_unlock(&c->dep1.mutex);

        pthread_mutex_lock(&c->dep2.mutex);
        printf("[D2] TEMPERATURA = %d \n",c->dep2.temperatura);
        pthread_mutex_unlock(&c->dep2.mutex);

        ispec.tv_sec += c->periodo;
    }
}


int main(){
    mlockall(MCL_CURRENT | MCL_FUTURE);
    srand(time(NULL));
    Control c;
    c.dep1.periodo = 2;
    c.dep2.periodo = 3;
    c.periodo = 4;

    pthread_mutex_init(&c.random,NULL);
    createMutex(&c.dep1.mutex);
    createMutex(&c.dep2.mutex);

    changePriority(SCHED_FIFO,30);
    createThread(&c.dep1.thread,&c,29,deposito1);
    createThread(&c.dep2.thread,&c,27,deposito2);
    createThread(&c.monitor,&c,25,monitor);

    pthread_join(c.monitor,NULL);
    pthread_join(c.dep1.thread,NULL);
    pthread_join(c.dep2.thread,NULL);

    pthread_mutex_destroy(&c.dep1.mutex);
    pthread_mutex_destroy(&c.dep2.mutex);
    pthread_mutex_destroy(&c.random);

    return 0;




}
