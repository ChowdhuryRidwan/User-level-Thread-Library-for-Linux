#include "nitwthread.h"

tcb_queue_ptr ready_q = NULL;
completed_tcb_queue_ptr dead_q = NULL;
struct sigaction alarm_handle;
extern ucontext_t notifier_context;
struct itimerval timer;
sigset_t sig_proc_mask;
extern int ignore_signal;

int nitwthread_init()
{
    if(ready_q == NULL && dead_q == NULL)
    {
        sigemptyset(&sig_proc_mask);
        sigaddset(&sig_proc_mask, SIGPROF);
        ignore_signal = 0;
        
        dead_q = (completed_tcb_queue_ptr)
                            malloc(sizeof(struct completed_tcb_queue));
        if(dead_q == NULL)
            return NOT_ENOUGH_MEMORY;
        dead_q->count = 0;
        dead_q->node = NULL;
        
        ready_q = (tcb_queue_ptr)malloc(sizeof(struct tcb_queue));
        if(ready_q == NULL)
            return NOT_ENOUGH_MEMORY;
        ready_q->count = 0;
        ready_q->head = ready_q->head_parent = NULL;
        
        nitwthread_tcb_ptr main_tcb = get_new_tcb_without_stack();
        if(main_tcb == NULL)
            return NOT_ENOUGH_MEMORY;
        getcontext(&(main_tcb->thr_context));
        get_fibre_completed_notifier_context();
        main_tcb->thr_context.uc_link = &notifier_context;
        
        enque_into_tcb_queue(ready_q, main_tcb);
        
        //set signal handler to call the scheduler
        sigemptyset(&alarm_handle.sa_mask);
        alarm_handle.sa_flags = 0;
        alarm_handle.sa_handler = &timer_tick_handler;
        sigaction(SIGPROF, &alarm_handle, NULL);
        
        //setup and start the timer
        timer.it_value.tv_sec = 0;
        timer.it_value.tv_usec = TIME_SLICE_VAL;
        timer.it_interval.tv_sec = 0;
        timer.it_interval.tv_usec = TIME_SLICE_VAL;
        setitimer(ITIMER_PROF, &timer, NULL);
    }
    return SUCCESS;
}

//wrapper function for threads
//pushes thread into completed queue on completion
//sets proper return value
void *wrapper_function(void *(*start_routine)(void *), void *arg)
{
    void *ret_val;
    nitwthread_tcb_ptr curr = get_current_tcb_ptr_atomically();
    ret_val = (*start_routine)(arg);
    sigprocmask(SIG_BLOCK, &sig_proc_mask, NULL);
    dead_node_ptr completed_thread_node =(dead_node_ptr)
                                        malloc(sizeof(struct dead_node));
    completed_thread_node->ret_code = (void**)malloc(sizeof(void*));
    completed_thread_node->next = NULL;
    *(completed_thread_node->ret_code) = NULL;
    if (completed_thread_node != NULL) 
    {
        *(completed_thread_node->ret_code) = ret_val;
        completed_thread_node->thr_id = curr->thr_id;
        // enque this to dead_q
        completed_thread_node->next = dead_q->node;
        dead_q->node = completed_thread_node;
    }
    sigprocmask(SIG_UNBLOCK, &sig_proc_mask, NULL);
    return ret_val;
}

int nitwthread_create(nitwthread_t *thread_id, nitwthread_attr_t *attr, 
                                void *(*start_routine)(void*), void* arg)
{
    if(ready_q == NULL)
        nitwthread_init();
    
    sigprocmask(SIG_BLOCK, &sig_proc_mask, NULL);
    nitwthread_tcb_ptr tcb = get_new_tcb_without_stack();
    if(tcb == NULL)
        return NOT_ENOUGH_MEMORY;
    getcontext(&(tcb->thr_context));
    tcb->thr_context.uc_stack.ss_sp = malloc(THREAD_STACK_SIZE);
    if(tcb->thr_context.uc_stack.ss_sp == NULL)
    {
        //Memory Exhaustion
        free(tcb);
        sigprocmask(SIG_UNBLOCK, &sig_proc_mask, NULL);
        return STACK_ALLOCATION_FAILED;
    }
    tcb->thr_context.uc_stack.ss_flags = 0;
    tcb->thr_context.uc_stack.ss_size = THREAD_STACK_SIZE;
    tcb->thr_context.uc_link = &notifier_context;
    makecontext(&(tcb->thr_context), 
                    (void (*)(void))wrapper_function, 2, start_routine, arg);
    
    *thread_id = tcb->thr_id;
    enque_into_tcb_queue(ready_q, tcb);
    sigprocmask(SIG_UNBLOCK, &sig_proc_mask, NULL);
    
    return SUCCESS;
}

int nitwthread_join(nitwthread_t thread, void **status)
{
    sigprocmask(SIG_BLOCK, &sig_proc_mask, NULL);
    nitwthread_tcb_ptr curr_tcb = get_current_tcb();
    nitwthread_tcb_ptr target_tcb = get_tcb_by_id(thread);
    if(curr_tcb == target_tcb || curr_tcb == NULL)
    {
        sigprocmask(SIG_UNBLOCK, &sig_proc_mask, NULL);
        return INVALID_OPERATION;
    }
    
    //if target not present in ready_q
    //check the completed queue
    if(target_tcb == NULL)
    {
        dead_node_ptr node = get_dead_node_by_id(thread, 1);
        sigprocmask(SIG_UNBLOCK, &sig_proc_mask, NULL);
        if(node != NULL)
        {
            if(status)
                *status = *node->ret_code;
            free(node);
            return SUCCESS;
        }
        else
            return INVALID_OPERATION;
    }
    
    //target is found
    //add current thread to target thread blocked list
    //wait for target thread to complete
    waiting_list_ptr node = (waiting_list_ptr)
                            malloc(sizeof(struct waiting_tcb_list));
    node->head = curr_tcb;
    node->next = target_tcb->blockedthr;
    target_tcb->blockedthr = node;
    curr_tcb->state = BLOCKED;
    sigprocmask(SIG_UNBLOCK, &sig_proc_mask, NULL);
    int state = curr_tcb->state;
    while(state == BLOCKED)
    {
        nitwthread_yield();
        state = curr_tcb->state;
    }
    
    //target thread is dead now
    sigprocmask(SIG_BLOCK, &sig_proc_mask, NULL);
    dead_node_ptr dead_target = get_dead_node_by_id(thread, 1);
    sigprocmask(SIG_UNBLOCK, &sig_proc_mask, NULL);
    if(dead_target != NULL)
    {
        if(status)
            *status = *dead_target->ret_code;
        free(dead_target);
    }
    return SUCCESS;
}


void clear_blocked_threads(nitwthread_tcb_ptr target_node) {
    waiting_list_ptr blocked_list = target_node->blockedthr;

    //first unblock all the other threads waiting for this
    //thread to complete
    while (blocked_list != NULL) {
        blocked_list->head->state = READY;
        blocked_list = blocked_list->next;
    }

    //DEBUGGING
    printf("Thread canceled: %ld\n", target_node->thr_id);
    //ENDDEBUGGING

    //then we mark this thread as complete
    target_node->state = COMPLETED;
}

int nitwthread_cancel(nitwthread_t thread) 
{
    sigprocmask(SIG_BLOCK, &sig_proc_mask, NULL);
    nitwthread_tcb_ptr curr_node = get_current_tcb();
    if ((curr_node != NULL) && (curr_node->thr_id != thread)) 
    {
        nitwthread_tcb_ptr target_node = get_tcb_by_id(thread);
        if (target_node != NULL) {
            clear_blocked_threads(target_node);
            sigprocmask(SIG_UNBLOCK, &sig_proc_mask, NULL);
            return 0;
        }
        //Cannot Call Cancel on some non Existing thread
        sigprocmask(SIG_UNBLOCK, &sig_proc_mask, NULL);
        return INVALID_OPERATION;
    }
    //Cannot Call Cancel on Yourself
    sigprocmask(SIG_UNBLOCK, &sig_proc_mask, NULL);
    return INVALID_OPERATION;
}

void nitwthread_exit(void *retval)
{
    sigprocmask(SIG_BLOCK, &sig_proc_mask, NULL);
    nitwthread_tcb_ptr curr = get_current_tcb();
    dead_node_ptr completed_thread_node =(dead_node_ptr)
                                        malloc(sizeof(struct dead_node));
    completed_thread_node->ret_code = (void**)malloc(sizeof(void*));
    completed_thread_node->next = NULL;
    *(completed_thread_node->ret_code) = NULL;
    if (completed_thread_node != NULL && curr != NULL) {
        *(completed_thread_node->ret_code) = retval;
        completed_thread_node->thr_id = curr->thr_id;
        completed_thread_node->next = dead_q->node;
        dead_q->node = completed_thread_node;
        dead_q->count++;
    }
    ignore_signal = 1;
    fibre_completed_notifier();
    sigprocmask(SIG_UNBLOCK, &sig_proc_mask, NULL);
    ignore_signal = 0;
    raise(SIGPROF);
}

void nitwthread_yield(void)
{
    raise(SIGPROF);
}
