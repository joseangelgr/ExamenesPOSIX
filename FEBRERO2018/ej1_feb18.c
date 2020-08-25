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
#include <math.h>
//IMPORTANTE NO OLVIDAR INCLUIR -lm

#define VAL_PI 3.14159
#define TIMER_ALERTA_A (SIGRTMIN+1)
#define TIMER_ALERTA_B (SIGRTMIN+2)
#define ALERTA_A (SIGRTMIN+3)
#define ALERTA_B (SIGRTMIN+4)

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

static inline double calculo(pthread_mutex_t* mutex,int avisos) {
    double r;
    pthread_mutex_lock(mutex);
    r = 50 + 50 * sin(2*VAL_PI*0.01*avisos);
    pthread_mutex_unlock(mutex);
    return r;
}

typedef struct{
    pthread_t thread;
    int periodo;
    int estado;
    int AVISOS;
    double estadistica;
} Alerta;

typedef struct{
    Alerta alerta_A,alerta_B;
    pthread_mutex_t mutex,random;
} Sistema;

void* tarea_A(void* arg){
    Sistema* s= (Sistema*) arg;
    timer_t timer;
    create_timer(&timer, TIMER_ALERTA_A);

    sigset_t thread_set;
    sigaddset(&thread_set, TIMER_ALERTA_A);
    sigaddset(&thread_set, ALERTA_A);

    activate_timer(timer, s->alerta_A.periodo, 0);

    while(true){
        int info;
        sigwait(&thread_set, &info);

        if (info == ALERTA_A){
            pthread_mutex_lock(&s->mutex);
            s->alerta_A.estado = 1;
            pthread_mutex_unlock(&s->mutex);
        }

        if(info == TIMER_ALERTA_A) {
            pthread_mutex_lock(&s->mutex);
            s->alerta_A.AVISOS++;

            if(s->alerta_A.estado == 1){
                s->alerta_A.estadistica = calculo(&s->random,s->alerta_A.AVISOS);
                printf("ESTADISTICA A CALCULADA:%f \n",s->alerta_A.estadistica);
                if(s->alerta_A.estadistica < 75){
                    s->alerta_A.estado = 0;
                }
            }

            pthread_mutex_unlock(&s->mutex);

            kill(getpid(),ALERTA_B);

        }

    }

    timer_delete(timer);

}

void* tarea_B(void* arg){
    Sistema* s= (Sistema*) arg;
    timer_t timer;
    create_timer(&timer, TIMER_ALERTA_B);

    sigset_t thread_set;
    sigaddset(&thread_set, TIMER_ALERTA_B);
    sigaddset(&thread_set, ALERTA_B);
    //sigaddset(&thread_set, ALERTA_A);

    activate_timer(timer, s->alerta_B.periodo, 0);

    while(true){
        int info;
        sigwait(&thread_set, &info);

        if (info == ALERTA_B){
            pthread_mutex_lock(&s->mutex);
            s->alerta_B.estado = 1;
            pthread_mutex_unlock(&s->mutex);
        }

        if(info == TIMER_ALERTA_B) {
            pthread_mutex_lock(&s->mutex);
            s->alerta_B.AVISOS += 2;

            if(s->alerta_B.estado == 1){
                s->alerta_B.estadistica = calculo(&s->random,s->alerta_B.AVISOS);
                printf("ESTADISTICA B CALCULADA:%f \n",s->alerta_B.estadistica);
                if(s->alerta_B.estadistica >= 75){
                    s->alerta_B.estado = 0;
                }
            }

            pthread_mutex_unlock(&s->mutex);

            kill(getpid(),ALERTA_A);

        }

    }

    timer_delete(timer);

}

sigset_t sigset;

int main(){
    mlockall(MCL_CURRENT | MCL_FUTURE);
    sigemptyset(&sigset); /* INICIALIZO EL SET DE SEÑALES VACÍO*/
    sigaddset(&sigset, TIMER_ALERTA_A);
    sigaddset(&sigset, TIMER_ALERTA_B);
    sigaddset(&sigset, ALERTA_A);
    sigaddset(&sigset, ALERTA_B);

    pthread_sigmask(SIG_BLOCK, &sigset, NULL);

    Sistema s;

    s.alerta_A.periodo=1;
    s.alerta_A.AVISOS=0;
    s.alerta_A.estado=0;

    s.alerta_B.estado=1;
    s.alerta_B.periodo=2;
    s.alerta_B.AVISOS=0;

    pthread_mutex_init(&s.mutex, NULL);
    pthread_mutex_init(&s.random, NULL);

    pthread_create(&s.alerta_A.thread, NULL, tarea_A, &s);
    pthread_create(&s.alerta_B.thread, NULL, tarea_B, &s);

    pthread_join(s.alerta_A.thread, NULL);
    pthread_join(s.alerta_B.thread, NULL);

    pthread_mutex_destroy(&s.random);
    pthread_mutex_destroy(&s.mutex);

    return 0;

}



