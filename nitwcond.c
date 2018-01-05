#include"nitwcond.h"
#include"terror.h"

cond_q_ptr cond_q;
sigset_t sig_mask;
extern tcb_queue_ptr ready_q;
extern mutex_q_ptr mutex_q;

int nitwthread_cond_init(nitwthread_cond_t *cond)
{
    static long cond_id = 1;
    static int sig_mask_initialized = 0;
    if(sig_mask_initialized == 0)
    {
        sigemptyset(&sig_mask);
        sigaddset(&sig_mask, SIGPROF);
        sig_mask_initialized = 1;
    }
    
    sigprocmask(SIG_BLOCK, &sig_mask, NULL);
    if(cond_q == NULL)
    {
        //Initialize the queue
        cond_q = (cond_q_ptr)malloc(sizeof(nitwthread_cond_queue));
        if(!cond_q)
            return NOT_ENOUGH_MEMORY;
        cond_q->head = NULL;
        cond_q->next = NULL;
    }
    
    nitwthread_cond_ptr new_cond = (nitwthread_cond_ptr)
                                        malloc(sizeof(nitwthread_cond));
    new_cond->wait_q = NULL;
    
    //enque into cond queue
    if(cond_q->head == NULL)
        cond_q->head = new_cond;
    else
    {
        cond_q_ptr it = cond_q;
        while(it->next != NULL)
            it = it->next;
        it->next = (cond_q_ptr)malloc(sizeof(struct nitwthread_cond_queue));
        if(it->next == NULL)
        {
            sigprocmask(SIG_UNBLOCK, &sig_mask, NULL);
            return NOT_ENOUGH_MEMORY;
        }
        it->next->head = new_cond;
        it->next->next = NULL;
    }
    
    new_cond->cond_id = cond_id++;
    *cond = new_cond->cond_id;
    sigprocmask(SIG_UNBLOCK, &sig_mask, NULL);
    return SUCCESS;
}

int nitwthread_cond_wait(nitwthread_cond_t *cond_id, nitwthread_mutex_t *mutex_id)
{
    sigprocmask(SIG_BLOCK, &sig_mask, NULL);
    
    cond_q_ptr iter = cond_q;
    while(iter != NULL && iter->head->cond_id != *cond_id)
        iter = iter->next;
    
    if(iter == NULL || nitwthread_mutex_unlock(mutex_id) != SUCCESS)
    {
        sigprocmask(SIG_UNBLOCK, &sig_mask, NULL);
        return INVALID_OPERATION;
    }
    
    nitwthread_cond_ptr cond = iter->head;
    nitwthread_tcb_ptr curr_thread = get_current_tcb();
    
    //put current thread into waiting list
    waiting_list_ptr wait_node = (waiting_list_ptr)
                                malloc(sizeof(struct waiting_tcb_list));
    if(wait_node == NULL)
    {
        sigprocmask(SIG_UNBLOCK, &sig_mask, NULL);
        return NOT_ENOUGH_MEMORY;
    }
    wait_node->head = curr_thread;
    wait_node->next = NULL;
    if(cond->wait_q == NULL)
        cond->wait_q = wait_node;
    else
    {
        waiting_list_ptr iter = cond->wait_q;
        while(iter->next != NULL)
            iter = iter->next;
        iter->next = wait_node;
    }
    
    curr_thread->state = BLOCKED;
    sigprocmask(SIG_UNBLOCK, &sig_mask, NULL);
    nitwthread_yield();
    
    nitwthread_mutex_lock(mutex_id);
    return SUCCESS;
}

int nitwthread_cond_signal(nitwthread_cond_t *cond_id)
{
    sigprocmask(SIG_BLOCK, &sig_mask, NULL);
    
    cond_q_ptr iter = cond_q;
    while(iter != NULL && iter->head->cond_id != *cond_id)
        iter = iter->next;
    if(iter == NULL)
    {
        sigprocmask(SIG_UNBLOCK, &sig_mask, NULL);
        return INVALID_OPERATION;
    }
    nitwthread_cond_ptr cond = iter->head;
    if(cond->wait_q != NULL)
    {
        nitwthread_tcb_ptr blocked_thread = cond->wait_q->head;
        cond->wait_q = cond->wait_q->next;
        blocked_thread->state = READY; 
    }
    sigprocmask(SIG_UNBLOCK, &sig_mask, NULL);
    return SUCCESS;
}