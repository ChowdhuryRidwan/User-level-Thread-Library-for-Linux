/* 
 * File:   nitwmutex.h
 * Author: Ridwan
 *
 */

#ifndef NITWMUTEX_H
#define NITWMUTEX_H

#include"nitwthread.h"

#ifdef __cplusplus
extern "C" {
#endif

    
typedef long nitwthread_mutex_t;
    
typedef struct nitwthread_mutex {
    nitwthread_mutex_t mutex_id; 
    nitwthread_tcb_ptr owner;
    waiting_list_ptr wait_q;
} nitwthread_mutex, *nitwthread_mutex_ptr;


typedef struct nitwthread_mutex_queue {
    nitwthread_mutex_ptr head;
    struct nitwthread_mutex_queue *next;
} nitwthread_mutex_queue, *mutex_q_ptr;

int  nitwthread_mutex_init(nitwthread_mutex_t *mutex);

int  nitwthread_mutex_lock(nitwthread_mutex_t *mutex);

int  nitwthread_mutex_unlock(nitwthread_mutex_t *mutex);


#ifdef __cplusplus
}
#endif

#endif /* NITWMUTEX_H */

