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

#define N 10
#define INIX 0
#define FINX 5
#define INIY 4
#define FINY 9
#define INIZ 1
#define FINZ 3

#define TIMER1 SIGRTMIN
#define TIMER2 (SIGRTMIN + 1)
#define TIMER3 (SIGRTMIN + 2)


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

//generar int aleatorio
static inline int generate_random(pthread_mutex_t* mutex) {
    int r;
    pthread_mutex_lock(mutex);
    r = random() % (N+1);
    pthread_mutex_unlock(mutex);
    return r;
}

static void zonaCritica(int x,int y, int z, int zona){
    if(x >= INIX && x<= FINX){
        if(y >= INIY && y <= FINY){
            if(z >= INIZ && z <= FINZ){
                //ZONA CORRECTA
            }else{
                printf("ZONA CRITICA %d en variable Z = %d [%d,%d] \n",zona,z,INIZ,FINZ);
            }
        }else{
            printf("ZONA CRITICA %d en variable Y = %d [%d,%d] \n",zona,y,INIY,FINY);
        }
    }else{
        printf("ZONA CRITICA %d en variable X = %d [%d,%d] \n",zona,x,INIX,FINX);
    }
}

typedef struct {
    pthread_t thread;
    int periodo;
} Tarea;

typedef struct {
    Tarea t1,t2,t3;
    int x,y,z;
    pthread_t monitor;
    pthread_mutex_t random, mutex;
} Control;

void* tarea1(void* arg){

    Control* control = (Control*) arg;

    timer_t timer;
    create_timer(&timer, TIMER1);


    sigset_t thread_set;
    sigaddset(&thread_set, TIMER1);

    //ACTIVAMOS EL TEMPORIZADOR
    activate_timer(timer, control->t1.periodo, 0);


    while(true) {
        int info;
        //La función sigwait () suspende la ejecución del hilo de llamada hasta que
        //       una de las señales especificadas en el conjunto de señales se convierte en pendiente.
        sigwait(&thread_set, &info);
        //devuelve por info el numero de la señal

        if(info == TIMER1) {

            kill(getpid(), TIMER1);

        }
    }
    /*Eliminamos el temporizador*/
    timer_delete(timer);

}

void* tarea2(void* arg){

    Control* control = (Control*) arg;

    timer_t timer;
    create_timer(&timer, TIMER2);


    sigset_t thread_set;
    sigaddset(&thread_set, TIMER2);

    //ACTIVAMOS EL TEMPORIZADOR
    activate_timer(timer, control->t2.periodo, 0);


    while(true) {
        int info;
        //La función sigwait () suspende la ejecución del hilo de llamada hasta que
        //       una de las señales especificadas en el conjunto de señales se convierte en pendiente.
        sigwait(&thread_set, &info);
        //devuelve por info el numero de la señal

        if(info == TIMER2) {

            kill(getpid(), TIMER2);
        }
    }
    /*Eliminamos el temporizador*/
    timer_delete(timer);

}

void* tarea3(void* arg){

    Control* control = (Control*) arg;

    timer_t timer;
    create_timer(&timer, TIMER3);


    sigset_t thread_set;
    sigaddset(&thread_set, TIMER3);

    //ACTIVAMOS EL TEMPORIZADOR
    activate_timer(timer, control->t3.periodo, 0);


    while(true) {
        int info;
        //La función sigwait () suspende la ejecución del hilo de llamada hasta que
        //       una de las señales especificadas en el conjunto de señales se convierte en pendiente.
        sigwait(&thread_set, &info);
        //devuelve por info el numero de la señal

        if(info == TIMER3) {
            kill(getpid(), TIMER3);
        }
    }
    /*Eliminamos el temporizador*/
    timer_delete(timer);

}

void* monitorizacion(void* arg) {
    Control* c = (Control*) arg;

    sigset_t thread_set;
    sigemptyset(&thread_set);
    sigaddset(&thread_set, TIMER1);
    sigaddset(&thread_set, TIMER2);
    sigaddset(&thread_set, TIMER3);

    while (true) {
        int info;
        sigwait(&thread_set, &info);//esperamos a que llegue una señal
        if (info == TIMER1) {

            pthread_mutex_lock(&c->mutex);

            c->x = generate_random(&c->random);
            c->y = generate_random(&c->random);
            c->z = generate_random(&c->random);
            zonaCritica(c->x, c->y, c->z, 1);

            pthread_mutex_unlock(&c->mutex);
        } else if (info == TIMER2) {//si llega COCHE

            pthread_mutex_lock(&c->mutex);

            c->x = generate_random(&c->random);
            c->y = generate_random(&c->random);
            c->z = generate_random(&c->random);
            zonaCritica(c->x, c->y, c->z, 2);

            pthread_mutex_unlock(&c->mutex);
        } else if (info == TIMER3) { //si llega FURGO
            pthread_mutex_lock(&c->mutex);

            c->x = generate_random(&c->random);
            c->y = generate_random(&c->random);
            c->z = generate_random(&c->random);
            zonaCritica(c->x, c->y, c->z, 3);

            pthread_mutex_unlock(&c->mutex);
        }
    }


}

sigset_t sigset;

int main(){
    mlockall(MCL_CURRENT | MCL_FUTURE);
    srandom(time(NULL));

    sigemptyset(&sigset);

    sigaddset(&sigset, TIMER1);
    sigaddset(&sigset, TIMER2);
    sigaddset(&sigset, TIMER3);

    pthread_sigmask(SIG_BLOCK, &sigset, NULL);


    Control c;
    c.x = 0;
    c.y = 0;
    c.z = 0;
    c.t1.periodo = 1;
    c.t2.periodo = 2;
    c.t3.periodo = 3;

    //Inicializamos los mutex
    pthread_mutex_init(&c.random, NULL);
    pthread_mutex_init(&c.mutex, NULL);


    pthread_create(&c.t1.thread, NULL, tarea1, &c);
    pthread_create(&c.t2.thread, NULL, tarea2, &c);
    pthread_create(&c.t3.thread, NULL, tarea3, &c);
    pthread_create(&c.monitor, NULL, monitorizacion, &c);

    pthread_join(c.t1.thread, NULL);
    pthread_join(c.t2.thread, NULL);
    pthread_join(c.t3.thread, NULL);
    pthread_join(c.monitor, NULL);

    pthread_mutex_destroy(&c.random);
    pthread_mutex_destroy(&c.mutex);
    return 0;

}