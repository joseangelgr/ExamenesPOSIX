//
// Created by Jose on 25/08/2020.
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
//con respecto a los ej1, no utilizamos signal.h(no se usan señales)

#define MAX 100
#define MIN 50

typedef struct{
    pthread_t thread;
    long valor;
    int periodo;
    pthread_mutex_t mutex;
}Detector;

typedef struct{
    Detector vibracion,presencia;
    pthread_mutex_t random;
    int periodo;
    pthread_t control;
}Sistema;

static int change_priority(int sched,int prio){
    struct sched_param param = {prio};
    return pthread_setschedparam(pthread_self(),sched,&param);
    //ATRIBUTOS(HEBRA ACTUAL,PLANIFICACION,SCHED_PARAM)
}

static inline long generate_random(pthread_mutex_t* mutex){
    long r;
    pthread_mutex_lock(mutex);
    r = random() % (MAX+MIN+1);//NUNCA LLEGA A MAYOR QUE MAX PUESTO COMO EN EL ENUNCIADO

    pthread_mutex_unlock(mutex);
    return r;
}

void* presencia_tarea(void* arg){
    Sistema * s = (Sistema*) arg;
    struct timespec ispec;
    clock_gettime(CLOCK_MONOTONIC, &ispec);
    ispec.tv_sec += s->presencia.periodo;

    while(true) {
        while(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ispec, NULL) == EINTR);
        pthread_mutex_lock(&s->presencia.mutex);
        s->presencia.valor = generate_random(&s->random);
        if(s->presencia.valor) {//es distinto de 0
            printf("Posible presencia\n");
        }
        else{
            printf("OK Presencia\n");
        }
        pthread_mutex_unlock(&s->presencia.mutex);
        ispec.tv_sec += s->presencia.periodo;
    }

}

void* vibracion_tarea(void* arg){
    Sistema * s = (Sistema*) arg;
    struct timespec ispec;
    clock_gettime(CLOCK_MONOTONIC, &ispec);
    ispec.tv_sec += s->vibracion.periodo;

    while(true) {
        while(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ispec, NULL) == EINTR);
        pthread_mutex_lock(&s->vibracion.mutex);
        s->vibracion.valor = generate_random(&s->random);
        if(s->vibracion.valor > MAX) {//es MAYOR QUE MAX
            printf("Posible entrada por ventana\n");
        }
        else{
            printf("OK VIBRACION \n");
        }
        pthread_mutex_unlock(&s->vibracion.mutex);
        ispec.tv_sec += s->vibracion.periodo;
    }

}

void* control_tarea(void* arg){
    Sistema * s = (Sistema*) arg;
    struct timespec ispec;
    clock_gettime(CLOCK_MONOTONIC, &ispec);
    ispec.tv_sec += s->periodo;

    while(true) {
        while(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ispec, NULL) == EINTR);
        pthread_mutex_lock(&s->vibracion.mutex);
        printf("Nivel de Vibracion: %ld \n",s->vibracion.valor);
        pthread_mutex_unlock(&s->vibracion.mutex);

        pthread_mutex_lock(&s->presencia.mutex);
        printf("Nivel de Presencia: %ld \n",s->presencia.valor);
        pthread_mutex_unlock(&s->presencia.mutex);

        ispec.tv_sec += s->periodo;
    }

}

static void crearHebraPresencia(pthread_t* hebra,void* arg,int prio){
    /*crear hebra presencia*/
    struct sched_param paramPresencia = {prio}; //EL MAS PRIORITARIO (29)
    pthread_attr_t attrPresencia;
    pthread_attr_init(&attrPresencia);
    //SET INHERITSCHED-POLITICA-PARAMETRO(PRIORIDAD)
    pthread_attr_setinheritsched(&attrPresencia,PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attrPresencia,SCHED_FIFO);
    pthread_attr_setschedparam(&attrPresencia, &paramPresencia);
    //create(&hebra,attributo,tarea,&struct)
    pthread_create(hebra,&attrPresencia,presencia_tarea,arg);
    pthread_attr_destroy(&attrPresencia);
}

static void crearHebraVibracion(pthread_t* hebra,void* arg,int prio){
    /*crear hebra vibracion*/
    struct sched_param paramVibracion = {prio};
    pthread_attr_t attrVibracion;
    pthread_attr_init(&attrVibracion);
    //SET INHERITSCHED-POLITICA-PARAMETRO(PRIORIDAD)
    pthread_attr_setinheritsched(&attrVibracion,PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attrVibracion,SCHED_FIFO);
    pthread_attr_setschedparam(&attrVibracion, &paramVibracion);
    //create(&hebra,attributo,tarea,&struct)
    pthread_create(hebra,&attrVibracion,vibracion_tarea,arg);
    pthread_attr_destroy(&attrVibracion);
}

static void crearHebraControl(pthread_t* hebra,void* arg,int prio){
    /*crear hebra control*/
    struct sched_param param = {prio};
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    //SET INHERITSCHED-POLITICA-PARAMETRO(PRIORIDAD)
    pthread_attr_setinheritsched(&attr,PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr,SCHED_FIFO);
    pthread_attr_setschedparam(&attr, &param);
    //create(&hebra,attributo,tarea,&struct)
    pthread_create(hebra,&attr,control_tarea,arg);
    pthread_attr_destroy(&attr);
}

//CREAR MUTEX

static int createMutex(pthread_mutex_t * mutex){
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT);
    int r = pthread_mutex_init(mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    return r;
}



int main(){
    mlockall(MCL_CURRENT | MCL_FUTURE);
    srand(time(NULL));
    Sistema s;
    s.vibracion.periodo = 3;
    s.presencia.periodo = 2;
    s.periodo = 4;

    pthread_mutex_init(&s.random,NULL);

    //CREAR LOS MUTEX DE LOS DETECTORES
    createMutex(&s.presencia.mutex);
    createMutex(&s.vibracion.mutex);

    /*el programa principal debe tener la maxima prioridad (30)*/
    change_priority(SCHED_FIFO,30);//planif. FIFO

    crearHebraPresencia(&s.presencia.thread,&s,29);
    crearHebraVibracion(&s.vibracion.thread,&s,27);
    crearHebraControl(&s.control,&s,25);


    pthread_join(s.control,NULL);
    pthread_join(s.vibracion.thread,NULL);
    pthread_join(s.presencia.thread,NULL);

    pthread_mutex_destroy(&s.presencia.mutex);
    pthread_mutex_destroy(&s.vibracion.mutex);
    pthread_mutex_destroy(&s.random);

    return 0;

}

