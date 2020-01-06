//provides exclusive mutual exclusion facility
#include"nitwmutex.h"

mutex_q_ptr mutex_q;
sigset_t sig_mask;
extern tcb_queue_ptr ready_q;

int  nitwthread_mutex_init(nitwthread_mutex_t *mutex)
{
    static long mutex_id = 1;
    static int sig_mask_initialized = 0;
    if(sig_mask_initialized == 0)
    {
        sigemptyset(&sig_mask);
        sigaddset(&sig_mask, SIGPROF);
        sig_mask_initialized = 1;
    }
    
    sigprocmask(SIG_BLOCK, &sig_mask, NULL);
    if(mutex_q == NULL)
    {
        //Initialize the queue
        mutex_q = (mutex_q_ptr)malloc(sizeof(nitwthread_mutex_queue));
        if(!mutex_q)
        {
            sigprocmask(SIG_UNBLOCK, &sig_mask, NULL);
            return NOT_ENOUGH_MEMORY;
        }
        mutex_q->head = NULL;
        mutex_q->next = NULL;
    }
    
    nitwthread_mutex_ptr new_mutex = (nitwthread_mutex_ptr)
                                        malloc(sizeof(nitwthread_mutex));
    new_mutex->owner = NULL;
    new_mutex->wait_q = (waiting_list_ptr)
                                malloc(sizeof(waiting_tcb_list));
    if(new_mutex->wait_q == NULL)
    {
        sigprocmask(SIG_UNBLOCK, &sig_mask, NULL);
        return NOT_ENOUGH_MEMORY;
    }
    new_mutex->wait_q->head = NULL;
    new_mutex->wait_q->next = NULL;
    
    //enque into the mutex queue
    if(mutex_q->head == NULL)
        mutex_q->head = new_mutex;
    else
    {
        mutex_q_ptr it = mutex_q;
        while(it->next != NULL)
            it = it->next;
        it->next = (mutex_q_ptr)malloc(sizeof(nitwthread_mutex_queue));
        if(it->next == NULL)
        {
            sigprocmask(SIG_UNBLOCK, &sig_mask, NULL);
            return NOT_ENOUGH_MEMORY;
        }
        it->next->head = new_mutex;
        it->next->next = NULL;
    }
    
    new_mutex->mutex_id = mutex_id++;
    *mutex = new_mutex->mutex_id;
    sigprocmask(SIG_UNBLOCK, &sig_mask, NULL);
    return SUCCESS;
}

int  nitwthread_mutex_lock(nitwthread_mutex_t *mutex_id)
{
    sigprocmask(SIG_BLOCK, &sig_mask, NULL);
    
    mutex_q_ptr it = mutex_q;
    while(it != NULL && it->head->mutex_id != *mutex_id)
        it = it->next;
    if(it == NULL)
    {
        sigprocmask(SIG_UNBLOCK, &sig_mask, NULL);
        return INVALID_OPERATION;
    }
    
    nitwthread_mutex_ptr mutex = it->head;
    nitwthread_tcb_ptr curr_thread = get_current_tcb();
    if(mutex->owner == NULL)
    {
        mutex->owner = curr_thread;
        sigprocmask(SIG_UNBLOCK, &sig_mask, NULL);
        return SUCCESS;
    }
    
    /*
    //remove current thread from ready queue
    ready_q->head_parent->thr_next = curr_thread->thr_next;
    ready_q->head = ready_q->head_parent;
    nitwthread_tcb_ptr iter = ready_q->head;
    while(iter->thr_next != ready_q->head)
        iter = iter->thr_next;
    ready_q->head_parent = iter;
    */
    
    //add current thread to the mutex wait queue
    if(mutex->wait_q == NULL)
    {
        mutex->wait_q = (waiting_list_ptr)
                                malloc(sizeof(waiting_tcb_list));
        mutex->wait_q->head = NULL;
        mutex->wait_q->next = NULL;
    }
    waiting_list_ptr wait_q = mutex->wait_q;
    if(wait_q->head == NULL)
        wait_q->head = curr_thread;
    else
    {
        while(wait_q->next != NULL)
            wait_q = wait_q->next;
        waiting_list_ptr wait_node = (waiting_list_ptr)
                                    malloc(sizeof(struct waiting_tcb_list));
        wait_node->head = curr_thread;
        wait_node->next = NULL;
        wait_q->next = wait_node;
    }
    curr_thread->state = BLOCKED;
    sigprocmask(SIG_UNBLOCK, &sig_mask, NULL);
    nitwthread_yield();
    
    return SUCCESS;
}

int  nitwthread_mutex_unlock(nitwthread_mutex_t *mutex_id)
{
    sigprocmask(SIG_BLOCK, &sig_mask, NULL);
    
    mutex_q_ptr it = mutex_q;
    while(it != NULL && it->head->mutex_id != *mutex_id)
        it = it->next;
    if(it == NULL)
    {
        sigprocmask(SIG_UNBLOCK, &sig_mask, NULL);
        return INVALID_OPERATION;
    }
    
    nitwthread_mutex_ptr mutex = it->head;
    nitwthread_tcb_ptr curr_thread = get_current_tcb();
    if(mutex->owner != curr_thread)
    {
        sigprocmask(SIG_UNBLOCK, &sig_mask, NULL);
        return INVALID_OPERATION;
    }
    mutex->owner = NULL;
    if(mutex->wait_q != NULL && mutex->wait_q->head != NULL)
    {
        nitwthread_tcb_ptr waking_up_thread = mutex->wait_q->head;
        waking_up_thread->state = READY;
        mutex->owner = waking_up_thread;
        mutex->wait_q = mutex->wait_q->next;
    }
    sigprocmask(SIG_UNBLOCK, &sig_mask, NULL);
    return SUCCESS;
}
