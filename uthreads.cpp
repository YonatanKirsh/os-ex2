//
// Created by yonat on 07-Apr-19.
//
// just checking
#include <stdio.h>
#include <stdlib.h>
#include <set>
#include <vector>
#include <sys/time.h>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>
#include "uthreads.h"

using namespace std;

enum State{READY, RUNNING, BLOCKED};  //todo no need?


sigjmp_buf env[MAX_THREAD_NUM];

#ifdef __x86_64__
/* code for 64 bit Intel arch */

typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%fs:0x30,%0\n"
                 "rol    $0x11,%0\n"
    : "=g" (ret)
    : "0" (addr));
    return ret;
}

#else
/* code for 32 bit Intel arch */

typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%gs:0x18,%0\n"
		"rol    $0x9,%0\n"
                 : "=g" (ret)
                 : "0" (addr));
    return ret;
}

#endif



/*
 * thread object, for internal use only
 */
class Thread
{
public:
    Thread(){};
    Thread(unsigned int id) : _id(id){}

    unsigned int _id = 0;
    char _stack[STACK_SIZE];
    //sumat with time
};


static unsigned int _q_usecs, _quantums = 0;
static set<int> _thread_ids;
vector<Thread*> _ready;
vector<Thread*> _blocked;
Thread _runningThread;



/*
 * Description: This function initializes the thread library.
 * You may assume that this function is called before any other thread library
 * function, and that it is called exactly once. The input to the function is
 * the length of a quantum in micro-seconds. It is an error to call this
 * function with non-positive quantum_usecs.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_init(int quantum_usecs)
{
    if (quantum_usecs <= 0)
    {
        //error message
        return -1;
    }
    _q_usecs = quantum_usecs;  //initialize global quantum usecs
    /*  ignore? user has to spawn main?
    thread_ids.insert(0);  //add main thread's id 0
    // todo main func? the one that initialized the library...?
    Thread main_thread(0);  //create main thread, add to threads list
    _ready.push_back(&main_thread);
    */
    return 0;
}

/*
 * Description: This function creates a new thread, whose entry point is the
 * function f with the signature void f(void). The thread is added to the end
 * of the READY threads list. The uthread_spawn function should fail if it
 * would cause the number of concurrent threads to exceed the limit
 * (MAX_THREAD_NUM). Each thread should be allocated with a stack of size
 * STACK_SIZE bytes.
 * Return value: On success, return the ID of the created thread.
 * On failure, return -1.
*/
int uthread_spawn(void (*f)(void))
{
    //find lowest unused id
    unsigned int i = 0;
    for (;(i < MAX_THREAD_NUM) && (_thread_ids.find(i) != _thread_ids.end()); ++i){}
    if (i == MAX_THREAD_NUM)
    {
        return -1; //todo print error?
    }
    Thread* newThread = new Thread(i);
    //setup new thread environment
    address_t sp, pc;
    sp = (address_t)newThread->_stack + STACK_SIZE - sizeof(address_t);
    pc = (address_t)f;
    sigsetjmp(env[i], 1);
    (env[i]->__jmpbuf)[JB_SP] = translate_address(sp);
    (env[i]->__jmpbuf)[JB_PC] = translate_address(pc);
    sigemptyset(&env[i]->__saved_mask);

    //todo add to ready list

}


/*
 * Description: This function terminates the thread with ID tid and deletes
 * it from all relevant control structures. All the resources allocated by
 * the library for this thread should be released. If no thread with ID tid
 * exists it is considered an error. Terminating the main thread
 * (tid == 0) will result in the termination of the entire process using
 * exit(0) [after releasing the assigned library memory].
 * Return value: The function returns 0 if the thread was successfully
 * terminated and -1 otherwise. If a thread terminates itself or the main
 * thread is terminated, the function does not return.
*/
int uthread_terminate(int tid);


/*
 * Description: This function blocks the thread with ID tid. The thread may
 * be resumed later using uthread_resume. If no thread with ID tid exists it
 * is considered as an error. In addition, it is an error to try blocking the
 * main thread (tid == 0). If a thread blocks itself, a scheduling decision
 * should be made. Blocking a thread in BLOCKED state has no
 * effect and is not considered an error.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_block(int tid);


/*
 * Description: This function resumes a blocked thread with ID tid and moves
 * it to the READY state. Resuming a thread in a RUNNING or READY state
 * has no effect and is not considered as an error. If no thread with
 * ID tid exists it is considered an error.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_resume(int tid);

/*
 * Description: This function blocks the RUNNING thread for usecs micro-seconds in real time (not virtual
 * time on the cpu). It is considered an error if the main thread (tid==0) calls this function. Immediately after
 * the RUNNING thread transitions to the BLOCKED state a scheduling decision should be made.
 * After the sleeping time is over, the thread should go back to the end of the READY threads list.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_sleep(unsigned int usec);


/*
 * Description: This function returns the thread ID of the calling thread.
 * Return value: The ID of the calling thread.
*/
int uthread_get_tid()
{
    return _runningThread._id;
}


/*
 * Description: This function returns the total number of quantums since
 * the library was initialized, including the current quantum.
 * Right after the call to uthread_init, the value should be 1.
 * Each time a new quantum starts, regardless of the reason, this number
 * should be increased by 1.
 * Return value: The total number of quantums.
*/
int uthread_get_total_quantums();


/*
 * Description: This function returns the number of quantums the thread with
 * ID tid was in RUNNING state. On the first time a thread runs, the function
 * should return 1. Every additional quantum that the thread starts should
 * increase this value by 1 (so if the thread with ID tid is in RUNNING state
 * when this function is called, include also the current quantum). If no
 * thread with ID tid exists it is considered an error.
 * Return value: On success, return the number of quantums of the thread with ID tid.
 * 			     On failure, return -1.
*/
int uthread_get_quantums(int tid);


