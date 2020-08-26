//
// Created by Jose on 26/08/2020.
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

#define N 100

#define MIN 0
#define MAX 100

typedef struct {
    pthread_t thread;
    int luminosidad;
    pthread_mutex_t mutex;
    int periodo;
}Sensor;

typedef struct {
    Sensor hab1,hab2;
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

void* habitacion1(void* arg){
    Control* c = (Control*) arg;
    struct timespec ispec;
    clock_gettime(CLOCK_MONOTONIC,&ispec);
    ispec.tv_sec += c->hab1.periodo;

    while(true){
        while(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ispec, NULL) == EINTR);
        pthread_mutex_lock(&c->hab1.mutex);
        c->hab1.luminosidad = generateRandom(&c->random);
        if(c->hab1.luminosidad < 0.8 * MAX ){
            printf("[H1] Activacion de luz artificial\n");
        }
        else if(c->hab1.luminosidad < 0.9 * MAX){
            printf("[H1] Luz optima\n");
        }

        else{
            printf("[H1] Activacion cortinas\n");
        }

        pthread_mutex_unlock(&c->hab1.mutex);
        ispec.tv_sec += c->hab1.periodo;
    }
}

void* habitacion2(void* arg){
    Control* c = (Control*) arg;
    struct timespec ispec;
    clock_gettime(CLOCK_MONOTONIC,&ispec);
    ispec.tv_sec += c->hab2.periodo;

    while(true){
        while(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ispec, NULL) == EINTR);
        pthread_mutex_lock(&c->hab2.mutex);
        c->hab2.luminosidad = generateRandom(&c->random);
        if(c->hab2.luminosidad < 0.8 * MAX ){
            printf("[H2] Activacion de luz artificial\n");
        }
        else if(c->hab2.luminosidad < 0.9 * MAX){
            printf("[H2] Luz optima\n");
        }

        else{
            printf("[H2] Activacion cortinas\n");
        }

        pthread_mutex_unlock(&c->hab2.mutex);
        ispec.tv_sec += c->hab2.periodo;
    }
}

void* monitor(void* arg){
    Control* c = (Control*) arg;
    struct timespec ispec;
    clock_gettime(CLOCK_MONOTONIC,&ispec);
    ispec.tv_sec += c->periodo;

    while(true){
        while(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ispec, NULL) == EINTR);

        pthread_mutex_lock(&c->hab1.mutex);
        printf("[H1] Luminosidad = %d \n",c->hab1.luminosidad);
        pthread_mutex_unlock(&c->hab1.mutex);

        pthread_mutex_lock(&c->hab2.mutex);
        printf("[H2] Luminosidad = %d \n",c->hab2.luminosidad);
        pthread_mutex_unlock(&c->hab2.mutex);

        ispec.tv_sec += c->periodo;
    }
}


int main(){
    mlockall(MCL_CURRENT | MCL_FUTURE);
    srand(time(NULL));
    Control c;
    c.hab1.periodo = 2;
    c.hab2.periodo = 3;
    c.periodo = 4;

    pthread_mutex_init(&c.random,NULL);
    createMutex(&c.hab1.mutex);
    createMutex(&c.hab2.mutex);

    changePriority(SCHED_FIFO,30);
    createThread(&c.hab1.thread,&c,29,habitacion1);
    createThread(&c.hab2.thread,&c,27,habitacion2);
    createThread(&c.monitor,&c,25,monitor);

    pthread_join(c.monitor,NULL);
    pthread_join(c.hab1.thread,NULL);
    pthread_join(c.hab2.thread,NULL);

    pthread_mutex_destroy(&c.hab1.mutex);
    pthread_mutex_destroy(&c.hab2.mutex);
    pthread_mutex_destroy(&c.random);

    return 0;




}
