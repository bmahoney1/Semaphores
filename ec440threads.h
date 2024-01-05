#include "semaphore.h"
//#include <semaphore.h>
#include <stdio.h> 
#include <stdlib.h> 
//#include <pthread.h>

#ifndef __EC440THREADS__
#define __EC440THREADS__



/*
unsigned long int ptr_demangle(unsigned long int p)
{   
    unsigned long int ret;

    asm("movq %1, %%rax;\n"
        "rorq $0x11, %%rax;"
        "xorq %%fs:0x30, %%rax;"
        "movq %%rax, %0;"
    : "=r"(ret)
    : "r"(p)
    : "%rax"
    );
    return ret;
}
*/

// HW 3 Function Headers //
void lock();
void unlock();
int pthread_join(pthread_t thread, void **value_ptr);
int sem_init(sem_t *sem, int pshared, unsigned value);
int sem_wait(sem_t *sem);
int sem_post(sem_t *sem);
int sem_destroy(sem_t *sem);

// HW 2 Function Headers //

unsigned long int ptr_demangle(unsigned long int p);


void thread_help();

/*
unsigned long int ptr_mangle(unsigned long int p)
{
    unsigned long int ret;

    asm("movq %1, %%rax;\n"
        "xorq %%fs:0x30, %%rax;"
        "rolq $0x11, %%rax;"
        "movq %%rax, %0;"
    : "=r"(ret)
    : "r"(p)
    : "%rax"
    );
    return ret;
}
*/
unsigned long int ptr_mangle(unsigned long int p);

void schedule();


int pthread_create(
	pthread_t *thread,
	const pthread_attr_t *attr,
	void *(*start_routine) (void *),
	void *arg	
);

void pthread_exit(void *value_ptr);

pthread_t pthread_self(void);

/*
void *start_thunk() {
  asm("popq %%rbp;\n"           //clean up the function prolog
      "movq %%r13, %%rdi;\n"    //put arg in $rdi
      "pushq %%r12;\n"          //push &start_routine
      "retq;\n"                 //return to &start_routine
      :
      :
      : "%rdi"
 );
        __builtin_unreachable();
}
*/
void *start_thunk();


#endif
