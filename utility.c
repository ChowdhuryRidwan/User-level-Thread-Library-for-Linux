#include "nitwthread.h"

static int id = 0;
extern sigset_t sig_proc_mask;
extern tcb_queue_ptr ready_q;
extern completed_tcb_queue_ptr dead_q;

nitwthread_tcb_ptr get_new_tcb_without_stack()
{
    nitwthread_tcb_ptr tcb = (nitwthread_tcb_ptr)
                                malloc(sizeof(struct nitwthread_tcb));
    
    if(tcb == NULL)
        return NULL;
    
    tcb->thr_id = id++;
    tcb->thr_context.uc_link = 0; // When the function associated with the current context completes its execution, the 
                                  // control transfers to the context pointed to by uc_link pointer.
    tcb->thr_context.uc_stack.ss_flags = 0; // controls the process context
    tcb->blockedthr = NULL;
    tcb->pid = getpid();
    tcb->state = READY;
    tcb->thr_usrpri = 20;
    tcb->thr_cpupri = 0;
    tcb->thr_totalcpu = 0;
    tcb->thr_next = NULL;
    
    return tcb;
}

nitwthread_tcb_ptr get_new_tcb()
{
    nitwthread_tcb_ptr tcb = (nitwthread_tcb_ptr)
                                malloc(sizeof(struct nitwthread_tcb));
    
    if(tcb == NULL)
        return NULL;
    
    tcb->thr_context.uc_stack.ss_sp = malloc(THREAD_STACK_SIZE);
    if(tcb->thr_context.uc_stack.ss_sp == NULL)
    {
        //Memory Exhaustion
        free(tcb);
        return NULL;
    }
    
    tcb->thr_id = id++;
    tcb->thr_context.uc_link = 0;
    tcb->thr_context.uc_stack.ss_flags = 0;
    tcb->blockedthr = NULL;
    tcb->pid = getpid();
    tcb->state = READY;
    tcb->thr_usrpri = 20;
    tcb->thr_cpupri = 0;
    tcb->thr_totalcpu = 0;
    tcb->thr_next = NULL;
    
    return tcb;
}

// Atomic operations allow for concurrent algorithms and access to certain shared data types without the use of mutexes.
nitwthread_tcb_ptr get_current_tcb_ptr_atomically()
{
    nitwthread_tcb_ptr tcb = NULL;
    sigprocmask(SIG_BLOCK, &sig_proc_mask, NULL);
    if(ready_q == NULL)
        tcb = NULL;
    else
        tcb = ready_q->head;
    sigprocmask(SIG_UNBLOCK, &sig_proc_mask, NULL);
    return tcb;
}

nitwthread_tcb_ptr get_current_tcb()
{
    if(ready_q == NULL)
        return NULL;
    return ready_q->head;
}

nitwthread_tcb_ptr get_tcb_by_id(nitwthread_t thr_id)
{
    nitwthread_tcb_ptr head = get_current_tcb();
    if(head == NULL)
        return NULL;
    if(head->thr_id == thr_id)
        return head;
    nitwthread_tcb_ptr it = head->thr_next;
    while(head != it)
    {
        if(it->thr_id == thr_id)
            return it;
        it = it->thr_next;
    }
    return NULL;
}

int enque_into_tcb_queue(tcb_queue_ptr q, nitwthread_tcb_ptr tcb)
{
    if(q == NULL || tcb == NULL)
        return INVALID_OPERATION;
    
    if(q->head == NULL)
    {
        //first tcb
        tcb->thr_next = tcb;   // circular list
        q->head_parent = tcb;
        q->head = tcb;
    }
    else
    {
        tcb->thr_next = q->head;
        q->head_parent->thr_next = tcb;
        q->head_parent = tcb;
    }
    q->count++;
    return SUCCESS;
}

int deque_from_ready_queue()
{
    nitwthread_tcb_ptr next_node,prev_node,head_node;
    if(ready_q == NULL)
        return INVALID_OPERATION;
    else
    {
        head_node = ready_q->head;
        prev_node = ready_q->head_parent;
        if(head_node != NULL)
        {

            next_node = head_node->thr_next;
            if(ready_q->count == 1)
            {
                //There is only one node in the queue
                //clear the pointers
                ready_q->head = ready_q->head_parent = NULL;
            }
            else
            {
                //More Than one node in queue
                //Make the parent node point to next_node
                ready_q->head = next_node;
                prev_node->thr_next = ready_q->head;
            }
            //free head node
            waiting_list_ptr blocked_tcb = head_node->blockedthr;
            if(head_node->thr_context.uc_stack.ss_sp != NULL)
            {
                free(head_node->thr_context.uc_stack.ss_sp);
            }
            while(blocked_tcb != NULL)
            {
                waiting_list_ptr next_tcb = blocked_tcb->next;
                free(blocked_tcb);
                blocked_tcb = next_tcb;
            }
            free(head_node);
            ready_q->count--;
        }
        else
            return INVALID_OPERATION;
    }
    return SUCCESS;
}

dead_node_ptr get_dead_node_by_id(nitwthread_t thr_id, int delete_node)
{
    if(dead_q != NULL && thr_id > 0)
    {
        dead_node_ptr curr_node = dead_q->node;
        dead_node_ptr prev_node = NULL;
        while(curr_node != NULL && curr_node->thr_id != thr_id)
        {
            prev_node = curr_node;
            curr_node = curr_node->next;
        }
        if(curr_node != NULL && delete_node != 0)
        {
            if(prev_node == NULL)
                dead_q->node = curr_node->next;
            else
                prev_node->next = curr_node->next;
            dead_q->count--;
        }
        return curr_node;
    }
    return NULL;
}
