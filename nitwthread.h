/*
 * File: nitwthread.h
 * Author: Ridwan
 */

#ifndef NITWTHREAD_H
#define NITWTHREAD_H

#include <stdio.h>
#include <ucontext.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <malloc.h>
#include "terror.h"
// encloses DLL's exported functions. this allows, function overloads, a C++ feature, not available in C language.
// the mechanism allows the same header to be used by both the C and C++ compiler
// Produces a standard C DLL (i.e. the function use the C language mangling scheme) when building the DLL.
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define MAX_THREADS_NUM 1000
#define TIME_SLICE_VAL 10000
#define THREAD_STACK_SIZE (1024*32)
#define READY 1001
#define BLOCKED 1002
#define RUNNING 1003
#define WAITING 1004
#define KILLED 1005
#define COMPLETED 1006

typedef long nitwthread_t, nitwthread_attr_t;

typedef struct nitwthread_tcb {
    nitwthread_t thr_id;                // Thread id value
    int thr_usrpri;                     // Thread default priority
    int thr_cpupri;                     // Priority calculated based on cpu time
    int thr_totalcpu;                   // Total cpu time in time quantums
    int state;                          // State of thread
    int pid;                            // Process id of thread owner
    ucontext_t thr_context;             // Thread context data structure
    struct waiting_tcb_list *blockedthr;  // List of blocked threads
    struct nitwthread_tcb *thr_next;  // Pointer to next thread in ready queue
} nitwthread_tcb, *nitwthread_tcb_ptr;


typedef struct nitwthread_attr {
    nitwthread_attr_t id;
    int attribute;
} *nitwthread_attr_ptr;


typedef struct tcb_queue {
    nitwthread_tcb_ptr head;
    nitwthread_tcb_ptr head_parent;
    long count;
} *tcb_queue_ptr;


typedef struct waiting_tcb_list {
    nitwthread_tcb_ptr head;
    struct waiting_tcb_list *next;
} waiting_tcb_list, *waiting_list_ptr;


typedef struct dead_node {
    nitwthread_t thr_id;	
    void **ret_code;
    struct dead_node* next;
} *dead_node_ptr;


typedef struct completed_tcb_queue {
    struct dead_node* node;
    long count;
} *completed_tcb_queue_ptr;



//Initialize the thread library.
//Will be called once at the time of
//creation of first thread
int nitwthread_init(void);

//create a new thread
//returns 0 on success
//otherwise proper error code. see terror.h
int nitwthread_create(nitwthread_t *thread_id, nitwthread_attr_t *attr, 
                                void *(*start_routine)(void*), void* arg);

//this function will be blocked till the thread referred by thread is complete
//if status is not null then the return value of thread will be stored here
//refer terror.h for more details
int nitwthread_join(nitwthread_t thread, void **status);

//this function will terminate the thread provided forcefully
//return value will be in accordance with terror.h
int  nitwthread_cancel(nitwthread_t thread);

//this function will return the time quantun assigned to it forcefully
void nitwthread_yield(void);

//this function will forcefully close the called thread
//the return value of this thread will be stored as retval
void nitwthread_exit(void *retval);


/*Scheduler functions*/

ucontext_t get_fibre_completed_notifier_context();

void fibre_completed_notifier();



/* Utility functions */

void timer_tick_handler();

int enque_into_tcb_queue(tcb_queue_ptr q, nitwthread_tcb_ptr tcb);

int deque_from_ready_queue();

nitwthread_tcb_ptr get_current_tcb();
nitwthread_tcb_ptr get_current_tcb_ptr_atomically();
nitwthread_tcb_ptr get_tcb_by_id(nitwthread_t thr_id);
nitwthread_tcb_ptr get_new_tcb_without_stack();
nitwthread_tcb_ptr get_new_tcb();

dead_node_ptr get_dead_node_by_id(nitwthread_t thr_id, int delete_node);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // NITWTHREAD_H_INCLUDED
