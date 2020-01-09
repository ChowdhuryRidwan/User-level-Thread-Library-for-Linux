# User-level-Thread-Library-for-Linux
The concept of threading within software programs is one of the most important mechanisms required for it to run effectively. Threading in a software allows it to run multiple tasks of a program at the same time. This is one of the concept a person needs to master in order to learn about Operating Systems. So with limited knowledge I went on to create my own thread library using C language. I used Context Switching method to create my library since this method gives the creator full control of scheduling (Round-Robin algorithm) the threads. This project was done out of curiosity during my leisure time. Completing this project has given me great satisfaction and an undeniable desire to create more projects in the future. 

Threads were created to make resource sharing simple and efficient. The concept is that a single process can have a number of threads, and everything is shared between them except the execution context (the stack and CPU registers). This way, if a shared resource is modified in one thread, the change is visible in all other threads. However, care must be taken to avoid problems where two threads try to access a shared resource at the same time. In this particular program the user is allowed to generate multiple tasks and our job is to schedule those tasks. 

In C, we have the datatype ucontext_t, for controlling the process context and it is actually a struct defined in the header file ucontext.h. The first one uc_link is a pointer to another context. When the function associated with the current context completes its execution, the control transfers to the context pointed to by uc_link pointer. If we want nothing to happen after the completion of current context, then simply keep it as NULL. We will be using Round-Robin scheduling, and so there must be a function that should execute after every time interval. This function should do all the necessary context switching and restoring. 

In the thread library, we define a ready_queue, which stores the pointers to the contexts of live threads like a linked list, task_count, that counts the number of alive threads. We also have initializing function, that initializes the signal handler for SIGPROF.

When a program is started, a mutex is created with a unique name. After this stage, any thread that needs the resource must lock the mutex from other threads while it is using the resource. The mutex is set to unlock when the data is no longer needed or the routine is finished.

A task_create function that create a context and attaches it with a function that was passed as argument to task_create and adds it into the ready_queue.

When the first task is created, the timer is set to start, and the signal handler function chooses one context from the ready_queue and it either sets or swaps with the current context.

There should also be a separate variable for storing the context of the main function, because we should note that the main function will also be scheduled just like the other tasks.

One more scenario can pan out in our program: The user created a new task and the create_task function was executing, in which the addition of the new task's context into the queue is going on. Now, imagine what happens when a SIGPROF is received? The control straight away transfers to the schedule function, but the queue was in middle of the pointer operations, and so this may lead to segmentation error.

Along these lines, we characterize a ready_queue, which stores every one of the pointers to the settings of live threads like a linked list, task_count, that tallies the quantity of alive threads, Initializing capacity, that instates the sign handler for SIGPROF (here, I utilize third method for tallying the time), set every one of the qualities for the caution. 

A task_create work that makes a unique circumstance and appends it with a capacity that was passed as contention to task_create and includes it into the ready_queue. 

At the point when the main undertaking is made, the clock is set to begin, and the sign handler capacity will be plan which picks one setting from the ready_queue and it either sets or swaps with the present context.



