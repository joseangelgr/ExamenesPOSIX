//
// Created by Jose on 25/08/2020.
//

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>

#define N 100
#define TIMER_ENTRADA SIGRTMIN
#define TIMER_SALIDA (SIGRTMIN+1)
#define ENTRADA (SIGRTMIN+2)
#define SALIDA (SIGRTMIN+3)

typedef struct{
    pthread_t thread;
    int periodo;

} Valvula;

typedef struct{
    Valvula entrada,salida;
    pthread_t control;
    int nivel;
    pthread_mutex_t mutex,random;
} Tanque;

static int create_timer(timer_t* timer, int signo) {
    struct sigevent sigev;
    sigev.sigev_notify = SIGEV_SIGNAL;
    sigev.sigev_signo = signo;
    sigev.sigev_value.sival_int = 0;
    return timer_create(CLOCK_MONOTONIC, &sigev, timer);
}

static int activate_timer(timer_t timer, time_t secs, long nsec) {
    struct itimerspec itspec;
    itspec.it_interval.tv_sec = secs;
    itspec.it_interval.tv_nsec = nsec;
    itspec.it_value.tv_sec = 0;
    itspec.it_value.tv_nsec = 1;
    return timer_settime(timer, 0, &itspec, NULL);
}

static inline int generate_random(pthread_mutex_t* mutex) {
    int r;
    pthread_mutex_lock(mutex);
    r = random() % (N+1);
    pthread_mutex_unlock(mutex);
    return r;
}

sigset_t sigset;

void* entrada_tarea(void* arg){
    Tanque* t = (Tanque*) arg;
    timer_t timer;
    create_timer(&timer, TIMER_ENTRADA);

    sigset_t thread_set;
    sigaddset(&thread_set, TIMER_ENTRADA);

    activate_timer(timer,t->entrada.periodo,0);

    while(true) {
        int info;
        int r;

        sigwait(&thread_set, &info);

        if(info == TIMER_ENTRADA) {
            pthread_mutex_lock(&t->mutex);
            r = generate_random(&t->random);

            if(r<60){ //Probabilidad 60%
                kill(getpid(), ENTRADA);
            }

            pthread_mutex_unlock(&t->mutex);

        }

    }

    timer_delete(timer);

}

void* salida_tarea(void* arg){
    Tanque* t = (Tanque*) arg;
    timer_t timer;
    create_timer(&timer, TIMER_SALIDA);

    sigset_t thread_set;
    sigaddset(&thread_set, TIMER_SALIDA);

    activate_timer(timer,t->salida.periodo,0);

    while(true) {
        int info;
        int r;

        sigwait(&thread_set, &info);

        if(info == TIMER_SALIDA) {

            r = generate_random(&t->random);

            if(r<40){ //Probabilidad 40%
                kill(getpid(),SALIDA);
            }

        }

    }

    timer_delete(timer);
}

void* monitor(void* arg){
    Tanque* t = (Tanque*) arg;
    sigset_t thread_set;
    sigemptyset(&thread_set);
    sigaddset(&thread_set, SALIDA);
    sigaddset(&thread_set, ENTRADA);

    while(true) {
        int info;
        sigwait(&thread_set, &info);
        pthread_mutex_lock(&t->mutex);
        if(info == ENTRADA) {
            t->nivel++;
            printf("Ha entrado 1 LITRO DE AGUA ++ NIVEL ACTUAL: %d litros\n", t->nivel);
        } else if (info == SALIDA) {
            t->nivel--;
            printf("Ha salido 1 LITRO DE AGUA -- NIVEL ACTUAL: %d litros\n", t->nivel);
        }
        pthread_mutex_unlock(&t->mutex);
    }
}

int main(){
    mlockall(MCL_CURRENT | MCL_FUTURE);
    srandom(time(NULL));

    sigemptyset(&sigset);
    sigaddset(&sigset, TIMER_ENTRADA);
    sigaddset(&sigset, TIMER_SALIDA);
    sigaddset(&sigset, SALIDA);
    sigaddset(&sigset, ENTRADA);

    pthread_sigmask(SIG_BLOCK, &sigset, NULL);

    Tanque t;
    t.nivel=0;
    t.entrada.periodo = 1;
    t.salida.periodo = 2;

    pthread_mutex_init(&t.random, NULL);
    pthread_mutex_init(&t.mutex, NULL);

    pthread_create(&t.entrada.thread, NULL, entrada_tarea, &t);
    pthread_create(&t.salida.thread, NULL, salida_tarea, &t);
    pthread_create(&t.control, NULL, monitor, &t);

    pthread_join(t.entrada.thread, NULL);
    pthread_join(t.salida.thread, NULL);
    pthread_join(t.control, NULL);

    pthread_mutex_destroy(&t.random);
    pthread_mutex_destroy(&t.mutex);

    return 0;


}