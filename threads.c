#include "ec440threads.h"
#include <setjmp.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
/* These are included with ec440threads.h
#include <stdio.h> 
#include <stdlib.h> 
#include <pthread.h>
*/

// Definitions //

#define MAX_NUM_THREADS 128


#define THREAD_STACK_SIZE 32767


#define QUANTUM 50


#define JB_RBX 0
#define JB_RBP 1
#define JB_R12 2
#define JB_R13 3
#define JB_R14 4
#define JB_R15 5
#define JB_RSP 6
#define JB_PC 7

// Thread Struct //

struct thread {
        int id;
        void *stack;
        jmp_buf regs;
        char status[30];
	int join;
	void * thread_exit;
//	int alarm;
};

// Global Variables //
struct thread Thread_Table[MAX_NUM_THREADS];// This creates the thread table which will store all the threads and their corresponding info
pthread_t runningThreadID = 0;
struct sigaction signal_handler;
int first = 1;
int threadcount = 0;
//sigset_t lock; // This holds the signals for lock
//sigset_t unlock; // This holds the signals for unlock


struct sems {
        int value;
	int current_value;
        int queue[MAX_NUM_THREADS];
        int flag;
	int qfirst;
	int qlast;
	int waiting;
};



// HW3 Functions //

int sem_init(sem_t *sem, int pshared, unsigned value){

	lock();
//	int i;
	struct sems *semaphore = malloc(sizeof(struct sems));
	sem->__align = (long int)semaphore;
	semaphore->current_value = value;
	semaphore->value = value;
	pshared = 0;
	semaphore->waiting = 0;
	semaphore->flag = 1;
	semaphore->qfirst = 0;
	semaphore->qlast = 0;	
//	for(i = 0; i< MAX_NUM_THREADS; i++){
 //       	semaphore->queue[i] = -1;
   //      }
	unlock();	
	return 0;	
}

int sem_wait(sem_t *sem){

	lock();	
	int current;
	struct sems *newsem = (struct sems*) sem->__align; 
	current = ((newsem)->current_value); // Checking to see if the value is 1 or 0
	
	if(current >0) {
		(newsem->current_value)--;
		unlock();
		return 0;
	}
	if (newsem->queue[newsem->qlast] == 0) {
		strcpy(Thread_Table[runningThreadID].status, "BLOCK!");
		newsem->queue[newsem->qlast] = Thread_Table[runningThreadID].id;
		newsem->qlast++;
		newsem->waiting++;
		unlock();
		schedule();
		lock();
	}
	unlock();

	return 0;

}

int sem_post(sem_t *sem){

	lock();
//	int current;
//	int i;
	struct sems *newsem = (struct sems*) sem->__align;
	
//	newsem->current_value++;
	newsem->current_value++;
	if(newsem->waiting!=0){
		newsem->waiting--;
		strcpy(Thread_Table[newsem->queue[newsem->qfirst]].status, "READY!");
		newsem->qfirst++;
//		for(i = 1; i< MAX_NUM_THREADS-1; i++){
 //               	newsem->queue[i-1] = newsem->queue[i];
  //       	}
//		newsem->queue[MAX_NUM_THREADS-1] = -1;
	}
	
	unlock();

	return 0;

}

int sem_destroy(sem_t *sem){

	lock();
	struct sems *newsem = (struct sems*) sem->__align;
	if (newsem->flag == 1) {
		free(newsem);		
	}
	unlock();

	return 0;
}


void lock(){
	sigset_t lock;  
  	sigaddset(&lock, SIGALRM); // This adds SIGALRM to lock, which will now be referenced by sigprocmask
  	sigprocmask(SIG_BLOCK, &lock, NULL); // This now makes it so whatever was within lock (SIGALRM) is now blocked and will not trigger the alarm
}

void unlock(){
	sigset_t unlock; 
  	sigaddset(&unlock,SIGALRM);
  	sigprocmask(SIG_UNBLOCK, &unlock, NULL);	
}


int pthread_join(pthread_t thread, void **value_ptr){
	lock();
	
	if (Thread_Table[thread].join == -1 && strcmp(Thread_Table[thread].status, "EXITED") != 0 && strcmp(Thread_Table[thread].status, "RUNNIN") != 0){
		strcpy(Thread_Table[runningThreadID].status, "BLOCK!"); // The currently running thread is now blocked
		Thread_Table[thread].join = Thread_Table[runningThreadID].id; // This now joins the two threads, storing the thread id into .thread of the joined thread to be referenced when it exits (so then the blocked thread can unblock)	
		unlock();
		schedule();
	}else{
		unlock();
	}
	lock();
	if(value_ptr != NULL) {
                *value_ptr = Thread_Table[thread].thread_exit;
        }
	unlock();
//	unlock();
	//schedule(); // Now all this is done, we continue on with scheduling
//	unlock();
	return 0;
		
}

void pthread_exit_wrapper(){
	unsigned long int res;
	asm("movq %%rax, %0\n":"=r"(res));
	pthread_exit((void *) res);
}


// HW2 Functions //
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
void setup(){
	int i;
//	lock();	
	for( i = 0; i < MAX_NUM_THREADS; i++){
		strcpy(Thread_Table[i].status, "EMPTY!");
		Thread_Table[i].id = i;
		Thread_Table[i].join = -1;
		Thread_Table[i].thread_exit = NULL;
	}
//	unlock();
	//      setjmp(Thread_Table[0].regs);   
//      runningThreadID++;
        
//      threadcount++;  
        // Setting up the ualarm to go off every 50 milliseconds, forcing a schedule
	
	useconds_t usecs = QUANTUM * 1000; // How long until the initial alarm is set off
	useconds_t interval = QUANTUM * 1000;// The intervals in which the alarm will continue to go off

	ualarm(usecs, interval);// Calls the ualarm function

	// Handling the sigalarm generated by ualarm
	sigemptyset(&signal_handler.sa_mask);
	signal_handler.sa_handler = &schedule;
	signal_handler.sa_flags = SA_NODEFER;	
	sigaction(SIGALRM, &signal_handler, NULL);
	
}

int pthread_create(
	pthread_t *thread, const pthread_attr_t *attr,
	void *(*start_routine) (void *), void *arg)
{
	lock();	
	attr = NULL;
	if (first){ // Checking to see if it is the first time pthread_create is being called
		setup(); // This sets up all the threads
		first = 0; // This makes it so that this never runs again (global)
		threadcount++;
		strcpy(Thread_Table[0].status,"READY!"); // This sets the main thread to ready
	}
	
		int i;
		for (i = 0; i< MAX_NUM_THREADS; i++){
        		if (strcmp(Thread_Table[i].status, "EMPTY!") == 0){
                		Thread_Table[i].id = i; // This sets the thread ID
                                *thread = i; // In the project description is says to set the *thread to the id #                 
                                Thread_Table[i].stack = malloc(THREAD_STACK_SIZE);
                                printf("Thread id: %d was made!\n", Thread_Table[i].id);
                                break;
                        }
                        if (i == MAX_NUM_THREADS){
                                printf("Error: The maximum number of threads were created!\n");
                                exit(EXIT_FAILURE);
                        }
                }
	threadcount++;

      // setjmp(Thread_Table[i].regs);
        Thread_Table[i].regs[0].__jmpbuf[JB_PC] = ptr_mangle((unsigned long int)start_thunk);// This function moves arginto start_routine which should start the thread 
		
        Thread_Table[i].regs[0].__jmpbuf[JB_R13] = (long unsigned int) arg;// This is the argument that start_routine takes  

        Thread_Table[i].regs[0].__jmpbuf[JB_R12] = (unsigned long int) start_routine;// This is what simulates the thread is doing something
		

//	 *(unsigned long int *)(Thread_Table[i].stack + THREAD_STACK_SIZE - 8) = (unsigned long int) pthread_exit;// Sets pthread_exit to the top of the stack, so when it returns to the top (finishes) it will exit

	 *(unsigned long int *)(Thread_Table[i].stack + THREAD_STACK_SIZE - 8) = (unsigned long int) pthread_exit_wrapper;

        Thread_Table[i].regs[0].__jmpbuf[JB_RSP] = ptr_mangle((unsigned long int)Thread_Table[i].stack + THREAD_STACK_SIZE -8);// This sets the stack pointer to the top of the thread

        strcpy(Thread_Table[i].status,"READY!");// Now that all the above is done, we can set the thread as Ready to be used
	unlock();
	
	return 0;
}

void pthread_exit(void *value_ptr){
	lock();
	
	// Since the joined thread is now exiting, the original thread is no longer blocked so we need to set it back to ready
	if(Thread_Table[runningThreadID].join != -1 && strcmp(Thread_Table[Thread_Table[runningThreadID].join].status, "READY!") != 0){
		strcpy(Thread_Table[Thread_Table[runningThreadID].join].status, "READY!");
	}
	
	Thread_Table[runningThreadID].thread_exit = value_ptr;

	strcpy(Thread_Table[runningThreadID].status, "EXITED");
	threadcount--;

	if(strcmp(Thread_Table[runningThreadID].status,"EXITED") == 0){
                        free(Thread_Table[runningThreadID].stack);
        }

	unlock();

	if(threadcount != 0){
                schedule();
        }

	exit(0);

	__builtin_unreachable();
}

pthread_t pthread_self(void){
        return runningThreadID;
}

void schedule(){
	//printf("Hello: %d\n", threadcount);
        // schedule() variables // 
        //if (threadcount == 0){
        //      exit(0);

        //}
	lock();

        if(strcmp(Thread_Table[runningThreadID].status, "EXITED") != 0 && strcmp(Thread_Table[runningThreadID].status, "BLOCK!") != 0){
                strcpy(Thread_Table[runningThreadID].status,"READY!");
        }
	
        pthread_t newID = runningThreadID;
        while(1){
                if(newID == MAX_NUM_THREADS - 1){
                        newID = 0;
                }
                else{
                        newID++;// Increment the id number
                }

                if(strcmp(Thread_Table[newID].status, "READY!") == 0){
                        break;
                }
        }

        int save = 0;// I use this variable to save the setjump location
        if(strcmp(Thread_Table[runningThreadID].status, "EXITED") != 0){
                save = setjmp(Thread_Table[runningThreadID].regs);

        }
	unlock();
	// When the same thread above is schedules and comes back to this point, save = 1 (the second argument in longjump) therefore will skip over it
        if(save != 1){
                runningThreadID = newID;
                strcpy(Thread_Table[runningThreadID].status, "RUNNIN");
                longjmp(Thread_Table[runningThreadID].regs, 1);
        }
}

