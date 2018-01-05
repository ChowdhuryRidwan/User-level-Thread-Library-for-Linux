#ifndef TERROR_H
#define TERROR_H

#ifdef	__cplusplus
extern "C" {
#endif

//These are the possible error codes returned

enum Terror
{
    //this will be returned if the requested function is successful
    SUCCESS = 0,

    //this will be returned if the provided
    //nitwthread_t parameter is invalid or not initialized
    INVALID_NITWTHREAD_T = -1,

    //this will be returned if memory cannot be alloted for stack
    STACK_ALLOCATION_FAILED = -2,

    //this will be returned if the requested operation
    //cannot be performed with the required parameters
    INVALID_OPERATION = -3,

    //this will be returned if unable to initialize
    //the thread library
    NOT_ENOUGH_MEMORY = -4
            
};


#ifdef	__cplusplus
}
#endif


#endif // TERROR_H