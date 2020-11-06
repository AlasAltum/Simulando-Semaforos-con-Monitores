#include <nSystem.h>
#include <stdio.h>

#include "fifoqueues.h"

#define TICKETS 3

/* 
Simulando semáforos con monitores. Los semáforos constan de dos funcionalidades:

* nWaitSem(nSem s) 
Que recibe un ticket para permitir que un thread siga ejecutando el código, permitiendo sincronización.
Si el semáforo no tiene tickets, el thread espera a que otro ejecute nSignalSem(nSem s) para entregar un ticket.

* nSignalSem(nSem s) 
Deposita un ticket en el semáforo.

Lo que haremos será simular que cada ticket es un monitor. Para esto implementaremos 3 FifoQueues. Pueden ser dos, pero
se utilizan 3 para ahorrar memoria.
*/

typedef struct Sem {
    nMonitor general;
    FifoQueue monitors;
    FifoQueue waiting;
    FifoQueue used;
} *nSem;

typedef struct {
    nMonitor m;
    int rdy;
} Pair;

nSem sem;


void initSem(nSem s){
    s = (nSem) nMalloc(sizeof(*s));
    s->general = nMakeMonitor();
    s->monitors = MakeFifoQueue();
    s->waiting = MakeFifoQueue();
    s->used = MakeFifoQueue(); // This fifoqueue is not necessary, but it helps reusing memory.
}

nSem nMakeSem(int tickets){
    initSem(sem);
    for (int i = 0; i < tickets; i++)
        PutObj(sem->monitors, nMakeMonitor());

    return sem;
}

void nWaitSem(nSem s){
    nMonitor m;
    nEnter(s->general); //We have to avoid dataraces here.

    if  (!EmptyFifoQueue(s->monitors)){ //If there are tickets, get one.
        m = (nMonitor) GetObj(s->monitors);
        PutObj(s->used, m);
        nEnter(m); 
    }

    else { // If there are not any tickets, then wait until a signalSem
        m = nMakeMonitor();
        Pair p = (Pair) {m, FALSE}; 
        PutObj(s->waiting, &p);
        nEnter(m);

        while (p.rdy == FALSE) //while not ready
            nWait(m); //wait until a ticket is given

        /*
        if code reaches this line, then it is already free
        printf("monitor with direction %p is already free, m");
        */
    }

    nExit(s->general);
}

void nSignalSem(nSem s){
    nEnter(s->general);
    if (!EmptyFifoQueue(s->waiting)){ //If we want a FIFO standard for our semaphores
        // then we have to wake up in the same order they came. That's why we wake up first
        // the waiting ones and in order of arrival. 
        Pair* p = GetObj(s->waiting);
        p->rdy = TRUE; //wake up another thread
    }
    
    else if (!EmptyFifoQueue(s->used)) { //allow used monitor to be reused
        nMonitor m = (nMonitor) GetObj(s->used);
        PutObj(s->monitors, m);
        nExit(m);
    }

    else { 
        PutObj(s->monitors, nMakeMonitor()); //add another ticket
    }
    nExit(s->general);
}
