//
// Created by Jose on 24/08/2020.
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
#define TIMER_TORNO1 SIGRTMIN
#define TIMER_TORNO2 (SIGRTMIN+1)
#define ENTRADA (SIGRTMIN+2)
#define SALIDA (SIGRTMIN+3)

typedef struct{
    pthread_t thread;
    int periodo;

} Torno;

typedef struct{
    Torno entrada,salida;
    pthread_t control;
    int aforo;
    pthread_mutex_t mutex,random;
} Discoteca;

//crear temporizador
static int create_timer(timer_t* timer, int signo) {
    struct sigevent sigev;
    sigev.sigev_notify = SIGEV_SIGNAL;
    sigev.sigev_signo = signo;
    sigev.sigev_value.sival_int = 0;
    return timer_create(CLOCK_MONOTONIC, &sigev, timer);
}

//activar temporizador
static int activate_timer(timer_t timer, time_t secs, long nsec) {
    struct itimerspec itspec;
    itspec.it_interval.tv_sec = secs;
    itspec.it_interval.tv_nsec = nsec;
    itspec.it_value.tv_sec = 0;
    itspec.it_value.tv_nsec = 1;
    return timer_settime(timer, 0, &itspec, NULL);
}

//generar INT aleatorio
static inline int generate_random(pthread_mutex_t* mutex) {
    int r;
    pthread_mutex_lock(mutex);
    r = random() % (N+1);
    pthread_mutex_unlock(mutex);
    return r;
}



sigset_t sigset;

void* entrada_tarea(void* arg){
    Discoteca* d = (Discoteca*) arg;
    timer_t timer;
    create_timer(&timer, TIMER_TORNO1);

    sigset_t thread_set;
    sigaddset(&thread_set, TIMER_TORNO1);

    activate_timer(timer,d->entrada.periodo,0);

    while(true) {
        int info;
        int r;

        sigwait(&thread_set, &info);

        if(info == TIMER_TORNO1) {
            pthread_mutex_lock(&d->mutex);
            r = generate_random(&d->random);

            if(r<60){ //Probabilidad 60%
                kill(getpid(), ENTRADA);
            }

            pthread_mutex_unlock(&d->mutex);

        }

    }

    timer_delete(timer);

}

void* salida_tarea(void* arg){
    Discoteca* d = (Discoteca*) arg;
    timer_t timer;
    create_timer(&timer, TIMER_TORNO2);

    sigset_t thread_set;
    sigaddset(&thread_set, TIMER_TORNO2);

    activate_timer(timer,d->salida.periodo,0);

    while(true) {
        int info;
        int r;

        sigwait(&thread_set, &info);

        if(info == TIMER_TORNO2) {
            pthread_mutex_lock(&d->mutex);
            r = generate_random(&d->random);

            if(r<40){ //Probabilidad 40%
                kill(getpid(),SALIDA);
            }

            pthread_mutex_unlock(&d->mutex);

        }

    }

    timer_delete(timer);

}

void* monitor(void* arg){
    Discoteca* d = (Discoteca*) arg;
    sigset_t thread_set;
    sigemptyset(&thread_set);
    sigaddset(&thread_set, SALIDA);
    sigaddset(&thread_set, ENTRADA);

    while(true) {
        int info;
        sigwait(&thread_set, &info);
        pthread_mutex_lock(&d->mutex);
        if(info == ENTRADA) {
            d->aforo++;
            printf("Ha entrado una persona ++ AFORO: %d personas\n", d->aforo);
        } else if (info == SALIDA) {
            d->aforo--;
            printf("Ha salido una persona -- AFORO: %d personas\n", d->aforo);
        }
        pthread_mutex_unlock(&d->mutex);
    }
}

int main(){

    mlockall(MCL_CURRENT | MCL_FUTURE);
    srandom(time(NULL));

    sigemptyset(&sigset);
    sigaddset(&sigset, TIMER_TORNO1);
    sigaddset(&sigset, TIMER_TORNO2);
    sigaddset(&sigset, SALIDA);
    sigaddset(&sigset, ENTRADA);

    pthread_sigmask(SIG_BLOCK, &sigset, NULL);

    Discoteca d;
    d.aforo=0;
    d.entrada.periodo=1;
    d.salida.periodo=2;

    pthread_mutex_init(&d.random, NULL);
    pthread_mutex_init(&d.mutex, NULL);

    pthread_create(&d.entrada.thread, NULL, entrada_tarea, &d);
    pthread_create(&d.salida.thread, NULL, salida_tarea, &d);
    pthread_create(&d.control, NULL, monitor, &d);

    pthread_join(d.entrada.thread, NULL);
    pthread_join(d.salida.thread, NULL);
    pthread_join(d.control, NULL);

    pthread_mutex_destroy(&d.random);
    pthread_mutex_destroy(&d.mutex);

    return 0;


}