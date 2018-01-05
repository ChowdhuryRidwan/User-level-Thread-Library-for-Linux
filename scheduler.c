#include "nitwthread.h"

ucontext_t notifier_context;
int ignore_signal = 0;
extern sigset_t sig_proc_mask;
extern tcb_queue_ptr ready_q;

void rr_schedule()
{
    if(!ignore_signal)
    {
        sigprocmask(SIG_BLOCK, &sig_proc_mask, NULL);
        if(ready_q->count == 1)
        {
            //only one node nothing to do
            //if completed then exit
            nitwthread_tcb_ptr curr = get_current_tcb();
            if(curr->state == COMPLETED)
            {
                deque_from_ready_queue();
                sigprocmask(SIG_UNBLOCK, &sig_proc_mask, NULL);
                return;
            }
            //END CONDITION OF USER THREADS
        }
        else if(ready_q->count > 0)
        {
            nitwthread_tcb_ptr curr = get_current_tcb();
            int current_thread_completed = 0;
            if(curr != NULL)
            {
                nitwthread_tcb_ptr next_tcb;
                if(curr->state == COMPLETED)
                {
                    //Remove the Node if it has completed Execution
                    current_thread_completed = 1;
                    deque_from_ready_queue();
                }
                else
                {
                    //move to forward tcb
                    nitwthread_tcb_ptr current_head = ready_q->head;
                    if(current_head != NULL)
                    {
                        ready_q->head_parent = current_head;
                        ready_q->head = current_head->thr_next;
                    }
                }
                next_tcb = get_current_tcb();

                while((next_tcb != NULL) && 
                                (next_tcb->state == BLOCKED || 
                                            next_tcb->state == COMPLETED))
                {
                    if(next_tcb->state == COMPLETED)
                    {
                        deque_from_ready_queue();
                    }
                    else
                    {
                        //This means the thread is not completed but blocked
                        //we need to move forward
                        nitwthread_tcb_ptr current_head = ready_q->head;
                        if(current_head != NULL)
                        {
                            ready_q->head_parent = current_head;
                            ready_q->head = current_head->thr_next;
                        }
                    }
                    next_tcb = get_current_tcb();
                }
                if(next_tcb == NULL)
                {
                    //All Threads Completed
                    sigprocmask(SIG_UNBLOCK, &sig_proc_mask, NULL);
                    return;
                }
                if(next_tcb != curr)
                {
                    //if After removing all the completed TCBs
                    //there are more then one node remaining
                    //then Context Switch
                    //Swap the Context between currentNode and nextNode

                    sigprocmask(SIG_UNBLOCK, &sig_proc_mask, NULL);
                    if(current_thread_completed)
                        setcontext(&(next_tcb->thr_context));
                    else
                        swapcontext(&(curr->thr_context),
                                                    &(next_tcb->thr_context));
                }

            }
        }
        sigprocmask(SIG_UNBLOCK, &sig_proc_mask, NULL);
    }
}

void timer_tick_handler()
{
    rr_schedule();
}

void fibre_completed_notifier()
{
    nitwthread_tcb_ptr current_tcb = get_current_tcb_ptr_atomically();
    waiting_list_ptr blocked_threads = current_tcb->blockedthr;
    
    while(blocked_threads != NULL)
    {
        blocked_threads->head->state = READY;
        blocked_threads = blocked_threads->next;
    }
    
    printf("Thread completed: %ld\n", current_tcb->thr_id);
    
    current_tcb->state = COMPLETED;
    
    raise(SIGPROF);
}

ucontext_t get_fibre_completed_notifier_context()
{
    static int context_created = 0;
    
    if(!context_created)
    {
        getcontext(&notifier_context);
        notifier_context.uc_link = 0;
        notifier_context.uc_stack.ss_sp = malloc(THREAD_STACK_SIZE);
        notifier_context.uc_stack.ss_size = THREAD_STACK_SIZE;
        notifier_context.uc_stack.ss_flags = 0;
        makecontext(&notifier_context, 
                        (void (*) (void))&fibre_completed_notifier, 0);
        context_created = 1;
    }
    return notifier_context;
}