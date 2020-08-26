//
// Created by Jose on 26/08/2020.
//

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#define N 100

#define INIX 0
#define INIY 50
#define INIZ 75

#define FINX 50
#define FINY 75
#define FINZ 100



typedef struct {
    pthread_t thread;
    int periodo;
    pthread_mutex_t mutex;
    int x,y,z;
} Tarea;

typedef struct {
    Tarea t1,t2,t3;
    pthread_t monitor;
    pthread_mutex_t random;
    int periodo;

} Control;

static inline int generate_random(pthread_mutex_t* mutex) {
    int r;
    pthread_mutex_lock(mutex);
    r = random() % (N+1);
    pthread_mutex_unlock(mutex);
    return r;
}

static void falloEstructural(int x,int y, int z, int zona){
    if(x >= 0.5 * INIX && y >= 0.5 * INIY && z >= 0.5 * INIZ ){
        if(x <= 1.5 * FINX && y <= 1.5 * FINY && z <= 1.5 * FINZ ){
                printf("Estructura %d Correcta  (%d,%d,%d)\n",zona,x,y,z);
        }else{
            printf("Posible Fallo Estructural %d (%d,%d,%d) -> Rango MAXIMO = [%d,%d,%d] \n",zona,x,y,z,FINX,FINY,FINZ);
        }
    }else{
        printf("Posible Fallo Estructural %d (%d,%d,%d) -> Rango MINIMO = [%d,%d,%d] \n",zona,x,y,z,INIX,INIY,FINY);
    }
}


static int createMutex(pthread_mutex_t* mutex){
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setprotocol(&attr,PTHREAD_PRIO_INHERIT);
    int r = pthread_mutex_init(mutex,&attr);
    pthread_mutexattr_destroy(&attr);
    return r;

}

static int changePriority(int sched,int prio){
    struct sched_param param = {prio};
    return pthread_setschedparam(pthread_self(),sched,&param);
}

static void createThread(pthread_t* thread, void* arg, int prio, void* (zona)()){
    struct sched_param param = {prio};
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setinheritsched(&attr,PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr,SCHED_FIFO);
    pthread_attr_setschedparam(&attr, &param);

    pthread_create(thread,&attr,zona,arg);
    pthread_attr_destroy(&attr);

}

void* zona1(void* arg){
    Control* c = (Control*) arg;
    struct timespec ispec;
    clock_gettime(CLOCK_MONOTONIC,&ispec);
    ispec.tv_sec += c->t1.periodo;

    while(true){
        while(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ispec, NULL) == EINTR);
        pthread_mutex_lock(&c->t1.mutex);
        c->t1.x = generate_random(&c->random);
        c->t1.y = generate_random(&c->random);
        c->t1.z = generate_random(&c->random);
        pthread_mutex_unlock(&c->t1.mutex);
        ispec.tv_sec += c->t1.periodo;
    }
}

void* zona2(void* arg){
    Control* c = (Control*) arg;
    struct timespec ispec;
    clock_gettime(CLOCK_MONOTONIC,&ispec);
    ispec.tv_sec += c->t2.periodo;

    while(true){
        while(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ispec, NULL) == EINTR);
        pthread_mutex_lock(&c->t2.mutex);
        c->t2.x = generate_random(&c->random);
        c->t2.y = generate_random(&c->random);
        c->t2.z = generate_random(&c->random);
        pthread_mutex_unlock(&c->t2.mutex);
        ispec.tv_sec += c->t2.periodo;
    }
}

void* zona3(void* arg){
    Control* c = (Control*) arg;
    struct timespec ispec;
    clock_gettime(CLOCK_MONOTONIC,&ispec);
    ispec.tv_sec += c->t3.periodo;

    while(true){
        while(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ispec, NULL) == EINTR);
        pthread_mutex_lock(&c->t3.mutex);
        c->t3.x = generate_random(&c->random);
        c->t3.y = generate_random(&c->random);
        c->t3.z = generate_random(&c->random);
        pthread_mutex_unlock(&c->t3.mutex);
        ispec.tv_sec += c->t3.periodo;
    }
}

void* monitor(void* arg){
    Control* c = (Control*) arg;
    struct timespec ispec;
    clock_gettime(CLOCK_MONOTONIC,&ispec);
    ispec.tv_sec += c->periodo;

    while(true){
        while(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ispec, NULL) == EINTR);

        pthread_mutex_lock(&c->t1.mutex);
        falloEstructural(c->t1.x,c->t1.y,c->t1.z,1);
        pthread_mutex_unlock(&c->t1.mutex);

        pthread_mutex_lock(&c->t2.mutex);
        falloEstructural(c->t2.x,c->t2.y,c->t2.z,2);
        pthread_mutex_unlock(&c->t2.mutex);

        pthread_mutex_lock(&c->t3.mutex);
        falloEstructural(c->t3.x,c->t3.y,c->t3.z,3);
        pthread_mutex_unlock(&c->t3.mutex);

        ispec.tv_sec += c->periodo;
    }
}




int main(){
    mlockall(MCL_CURRENT | MCL_FUTURE);
    srand(time(NULL));
    Control c;
    c.t1.periodo = 2;
    c.t2.periodo = 3;
    c.t3.periodo = 4;
    c.periodo = 5;

    pthread_mutex_init(&c.random,NULL);
    createMutex(&c.t1.mutex);
    createMutex(&c.t2.mutex);
    createMutex(&c.t3.mutex);

    changePriority(SCHED_FIFO,30);
    createThread(&c.t1.thread,&c,29,zona1);
    createThread(&c.t2.thread,&c,27,zona2);
    createThread(&c.t3.thread,&c,25,zona3);
    createThread(&c.monitor,&c,23,monitor);

    pthread_join(c.monitor,NULL);
    pthread_join(c.t1.thread,NULL);
    pthread_join(c.t2.thread,NULL);
    pthread_join(c.t3.thread,NULL);

    pthread_mutex_destroy(&c.t1.mutex);
    pthread_mutex_destroy(&c.t2.mutex);
    pthread_mutex_destroy(&c.t3.mutex);
    pthread_mutex_destroy(&c.random);

    return 0;

}