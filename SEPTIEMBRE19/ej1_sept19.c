//
// Created by Jose on 24/08/2020.
//

/*EJERCICIO 1. TEMPORIZADORES*/
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>

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
//generar long aleatorio
static inline long generate_random(pthread_mutex_t* mutex) {
    long r;
    pthread_mutex_lock(mutex);
    r = random() % 2;
    pthread_mutex_unlock(mutex);
    return r;
}

/*CADA VEHICULO TIENE UNA HEBRA Y UN PERIODO*/
typedef struct {
    pthread_t thread;
    int periodo;
} Vehiculo;

/*tenemos dos vehiculos, una hebra de entrada y otra de salida...*/
/*El periodo de control del numero de vehiculos*/
/*dos mutex, UNO DE ELLOS PARA RANDOM*/
/*El numero de coches y furgos que hay*/
typedef struct {
    Vehiculo furgo, coche;
    pthread_t entrada;
    pthread_t salida;
    int periodo;
    pthread_mutex_t random, mutex;
    int coches, furgos;
} Control;


//SEÑALES DE ENTRADA/SALIDA Y TIMERS DE COCHES Y FURGO Y TIMER DE SALIDA
#define ENTRA_COCHE SIGRTMIN
#define SALE_COCHE (SIGRTMIN + 1)
#define ENTRA_FURGO (SIGRTMIN + 2)
#define SALE_FURGO (SIGRTMIN + 3)
#define TIMER_COCHE (SIGRTMIN + 4)
#define TIMER_FURGO (SIGRTMIN + 5)
#define TIMER_SALIDA (SIGRTMIN + 6)

//--TAREA DE COCHE--
void* coche_tarea(void* arg){
    //Le pasamos como argumento el struct Control
    Control* control = (Control*) arg;

    //DECLARAMOS EL TEMPORIZADOR
    timer_t timer;
    create_timer(&timer, TIMER_COCHE);

    //DECLARAMOS EL SET DE SEÑALES... EN ESTE SET NECESITAMOS EL TIMER DEL VEHICULO
    sigset_t thread_set;
    sigaddset(&thread_set, TIMER_COCHE);
    //ACTIVAMOS EL TEMPORIZADOR
    activate_timer(timer, control->coche.periodo, 0);


    while(true) {
        int info;
        //La función sigwait () suspende la ejecución del hilo de llamada hasta que
        //       una de las señales especificadas en el conjunto de señales se convierte en pendiente.
        sigwait(&thread_set, &info);
        //devuelve por info el numero de la señal

        if(info == TIMER_COCHE) { /*Si el numero de señal es el de TIMER_COCHE*/
            kill(getpid(), ENTRA_COCHE); /*Enviamos la señal de entrada de coche al proceso principal(getpid()) */
        }
    }
    /*Eliminamos el temporizador*/
    timer_delete(timer);



}
void* furgo_tarea(void* arg){
    //Le pasamos como argumento el struct Control
    Control* control = (Control*) arg;

    //DECLARAMOS EL TEMPORIZADOR
    timer_t timer;
    create_timer(&timer, TIMER_FURGO);

    //DECLARAMOS EL SET DE SEÑALES... EN ESTE SET NECESITAMOS EL TIMER DEL VEHICULO
    sigset_t thread_set;
    sigaddset(&thread_set, TIMER_FURGO);

    //ACTIVAMOS EL TEMPORIZADOR
    activate_timer(timer, control->furgo.periodo, 0);



    while(true) {
        int info;
        //La función sigwait () suspende la ejecución del hilo de llamada hasta que
        //       una de las señales especificadas en el conjunto de señales se convierte en pendiente.
        sigwait(&thread_set, &info);
        //devuelve por info el numero de la señal

        if(info == TIMER_FURGO) { /*Si el numero de señal es el de TIMER_FURGO*/
            kill(getpid(), ENTRA_FURGO); /*Enviamos la señal de entrada de furgo al proceso principal(getpid()) */
        }
    }
    /*Eliminamos el temporizador*/
    timer_delete(timer);
}

void* entrada_tarea(void* arg){
    Control* control= (Control*) arg;
    timer_t timer;
    sigset_t thread_set;//necesitamos las señales de entrada de los vehiculos y el Timer de salida
    sigemptyset(&thread_set);

    sigaddset(&thread_set, TIMER_SALIDA);
    sigaddset(&thread_set, ENTRA_COCHE);
    sigaddset(&thread_set, ENTRA_FURGO);

    create_timer(&timer, TIMER_SALIDA);//creo el timer con el timer de salida
    activate_timer(timer, control->furgo.periodo, 0);

    while(true) {
        int info;
        sigwait(&thread_set, &info);//esperamos a que llegue una señal
        if(info == TIMER_SALIDA) {//si llega TIMER_SALIDA
            long r = generate_random(&control->random);
            (r) ? kill(getpid(), SALE_COCHE): kill(getpid(), SALE_FURGO); // si el r generado es 1 enviamos la señal de salida del coche sino la de Furgo
        } else if(info == ENTRA_COCHE) {//si llega COCHE
            pthread_mutex_lock(&control->mutex);
            control->coches++;//incrementamos el numero de coches
            printf("Ha entrado un coche ++  Hay %d coches\n", control->coches);
            pthread_mutex_unlock(&control->mutex);
        } else if(info == ENTRA_FURGO) { //si llega FURGO
            pthread_mutex_lock(&control->mutex);//bloquear y desbloquear mutex para ex.mutua
            control->furgos++;//incrementamos el numero de furgos
            printf("Ha entrado una furgo ++ Hay %d furgos\n", control->furgos);
            pthread_mutex_unlock(&control->mutex);
        }
    }
    //borramos el timer
    timer_delete(timer);
}


void* salida_tarea(void* arg){

    Control* c = (Control*) arg;
    sigset_t thread_set;
    sigemptyset(&thread_set);
    sigaddset(&thread_set, SALE_COCHE);
    sigaddset(&thread_set, SALE_FURGO);

    while(true) {
        int info;
        sigwait(&thread_set, &info);
        pthread_mutex_lock(&c->mutex);
        if(info == SALE_COCHE && c->coches > 0) {
            c->coches--;
            printf("Ha salido un coche -- Hay %d coches\n", c->coches);
        } else if (info == SALE_FURGO && c->furgos > 0) {
            c->furgos--;
            printf("Ha salido una furgo -- Hay %d furgos\n", c->furgos);
        }
        pthread_mutex_unlock(&c->mutex);
    }

}

sigset_t sigset;
int main(){

    //Bloquear todas las páginas asignadas al espacio de direcciones del proceso de llamada
    //MCL_CURRENT -> Bloquear todas las páginas que están asignadas actualmente a la dirección
    //               espacio del proceso.
    //MCL_FUTURE -> Bloquear todas las páginas que se asignarán al espacio de direcciones
    //              del proceso en el futuro.
    mlockall(MCL_CURRENT | MCL_FUTURE);
    srandom(time(NULL)); /* Inicializando el generador de números aleatorios  */
    sigemptyset(&sigset); /* INICIALIZO EL SET DE SEÑALES VACÍO*/

    //Añado cada una de las señales al set de señales
    sigaddset(&sigset, TIMER_SALIDA);
    sigaddset(&sigset, ENTRA_COCHE);
    sigaddset(&sigset, SALE_COCHE);
    sigaddset(&sigset, ENTRA_FURGO);
    sigaddset(&sigset, SALE_FURGO);
    sigaddset(&sigset, TIMER_FURGO);
    sigaddset(&sigset, TIMER_COCHE);

    pthread_sigmask(SIG_BLOCK, &sigset, NULL); /*Examina y cambia la máscara de las señales bloqueadas*/

    Control c;
    c.coches = 0;//inicialmente tenemos 0 coches y 0 furgonetas
    c.furgos = 0;
    c.coche.periodo = 2;
    c.furgo.periodo = 3;
    c.periodo = 2;

    //Inicializamos los mutex
    pthread_mutex_init(&c.random, NULL);
    pthread_mutex_init(&c.mutex, NULL);

    //Crear hebras
    pthread_create(&c.coche.thread, NULL, coche_tarea, &c);
    pthread_create(&c.furgo.thread, NULL, furgo_tarea, &c);
    pthread_create(&c.entrada, NULL, entrada_tarea, &c);
    pthread_create(&c.salida, NULL, salida_tarea, &c);

    //Esperar hebras
    pthread_join(c.coche.thread, NULL);
    pthread_join(c.furgo.thread, NULL);
    pthread_join(c.entrada, NULL);
    pthread_join(c.salida, NULL);

    //Eliminar los mutex
    pthread_mutex_destroy(&c.random);
    pthread_mutex_destroy(&c.mutex);
    return 0;

}




