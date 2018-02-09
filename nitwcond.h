/* 
 * File:   nitwcond.h
 * Author: Ridwan
 *
 */

#ifndef NITWCOND_H
#define NITWCOND_H

#include"nitwmutex.h"

#ifdef __cplusplus
extern "C" {
#endif
    
typedef long nitwthread_cond_t;

typedef struct nitwthread_cond {
    nitwthread_cond_t cond_id;
    waiting_list_ptr wait_q;
} nitwthread_cond, *nitwthread_cond_ptr;

typedef struct nitwthread_cond_queue {
    nitwthread_cond_ptr head;
    struct nitwthread_cond_queue *next;
}nitwthread_cond_queue, *cond_q_ptr;


int nitwthread_cond_init(nitwthread_cond_t *cond);

int nitwthread_cond_wait(nitwthread_cond_t *cond, nitwthread_mutex_t *mutex);

int nitwthread_cond_signal(nitwthread_cond_t *cond);


#ifdef __cplusplus
}
#endif

#endif /* NITWCOND_H */

