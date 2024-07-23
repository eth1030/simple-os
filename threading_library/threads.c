#include "ec440threads.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <setjmp.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>

/* You can support more threads. At least support this many. */
#define MAX_THREADS 128

/* Your stack should be this many bytes in size */
#define THREAD_STACK_SIZE 32767

/* Number of microseconds between scheduling events */
#define SCHEDULER_INTERVAL_USECS (50 * 1000)

/* Extracted from private libc headers. These are not part of the public
 * interface for jmp_buf.
 */
#define JB_RBX 0
#define JB_RBP 1
#define JB_R12 2
#define JB_R13 3
#define JB_R14 4
#define JB_R15 5
#define JB_RSP 6
#define JB_PC 7

/* thread_status identifies the current state of a thread. You can add, rename,
 * or delete these values. This is only a suggestion. */
enum thread_status
{
	TS_EXITED,
	TS_RUNNING,
	TS_READY
};

/* The thread control block stores information about a thread. You will
 * need one of this per thread.
 */
struct thread_control_block {
	pthread_t tid;
	void *stackptr;
	jmp_buf myreg;
	enum thread_status status;
};

// global tcb variables
struct thread_control_block *TCBA[MAX_THREADS];
struct thread_control_block *current_t;

/* Queue functions and definitions
 */
struct node {
  		struct thread_control_block *ptr_t; // pointer to tcb
  		struct node *next;
	};

// global queue variables
struct node *head = NULL;
struct node *tail = NULL;
struct node *temp = NULL;

// initializes a new node to be scheduled
struct node *create_node(struct thread_control_block *pointer) {
	struct node *create = NULL;
	create = malloc(sizeof(struct node));
	create->ptr_t = pointer;
	create->next = NULL;
	return create;
}

// add a newly created node to the end of the queue
void addnode(struct node *newnode) {
	tail->next = newnode;
	tail = tail->next;
	tail->next = head;
}

// remove a node that has completed running from the head
void removenode() {
	// remove thread from queue
	temp = head;
	tail->next = head->next;
	head = head->next;
	// set TCBA arrary to NULL so new thread can replace exited id
	TCBA[temp->ptr_t->tid] = NULL;
}

// frees resources after thread stops running and exits
void freenode() {
	printf("bruh");
	// free stack
	if (temp->ptr_t->stackptr != NULL) {
		free((temp->ptr_t->stackptr) + 8 - THREAD_STACK_SIZE);
	}
	printf("bruh");
	// free TCB
	free(temp->ptr_t);
	printf("bruh");
	// free node
	free(temp);
	printf("bruh");
	temp = NULL;
}

// move node to back of the queue when its not ready to be run or its done running
void movenode() {
	tail = tail->next;
	head = head->next;
}

static void schedule(int signal) {
	 	// only long jump when set jmp succeeds
		int jmp = 0;
		if (head->ptr_t->status == TS_RUNNING) {
			// update jmpbuf
			jmp = sigsetjmp(head->ptr_t->myreg, 0);
			if (jmp == 1) {
				return;
			}
			head->ptr_t->status = TS_READY;
			movenode();
		}
		// determine which thread runs next
		while(1) {
			 if (head->ptr_t->status == TS_READY) {
				if (temp != NULL) {
					freenode();
				}
				break;
			} else if (head->ptr_t->status == TS_EXITED) {
				removenode();
			}
			if (head == tail) {
				break;
			}
			if (head == NULL) {
				// when schedule is called, all the resources in the thread will be freed
				// free main TCB when queue is empty
				free(TCBA[0]);
				exit(0);
			}
		}
		// switch to next thread
		if (jmp == 0) {
			current_t = head->ptr_t;
			head->ptr_t->status = TS_RUNNING;
			siglongjmp(head->ptr_t->myreg,0);
		}
}

void sig_handler(int signum) {
	// calls schedule when it catches SIGALRM
	if(signum == SIGALRM) {
		schedule(signum);
	}
}

static void scheduler_init()
{
	// initialize main TCB
	struct thread_control_block *ptr = malloc(sizeof(struct thread_control_block));
	// set first element of global TCB array
	TCBA[0] = ptr;
	ptr->tid = (pthread_t)0;
	ptr->stackptr = NULL;
	ptr->status = TS_RUNNING;
	// call setjmp so main can be turned into a thread
	if(sigsetjmp(ptr->myreg,0) == 1)
		return;
	// no need to modify current stack and registers
	// set up timer to call schedule()
	extern void sig_handler();
	struct sigaction act;
	// define handler
	act.sa_flags = SA_NODEFER;
	act.sa_handler = &sig_handler;
	sigemptyset(&act.sa_mask);
	ualarm((useconds_t)SCHEDULER_INTERVAL_USECS,SCHEDULER_INTERVAL_USECS);
	sigaction(SIGALRM,&act,NULL);
	// set main as current thread
	current_t = ptr;
	// schedule main
	struct node *first_q = create_node(TCBA[0]);
	head = first_q;
	tail = first_q;
	tail->next = head;
}

int pthread_create(
	pthread_t *thread, const pthread_attr_t *attr,
	void *(*start_routine) (void *), void *arg)
{
	// Create the timer and handler for the scheduler. Create thread 0.
	static bool is_first_call = true;
	if (is_first_call)
	{
		is_first_call = false;
		scheduler_init();
	}
	// create all other threads
	// give thread a unique tid	
	struct thread_control_block *ptr = malloc(sizeof(struct thread_control_block));
	if (ptr == NULL)
		return -1;
	int i = 1;
	while (TCBA[i]!= NULL) {	
		i++;
		if (i == MAX_THREADS) {
			break;
		}
	}
	if (i == MAX_THREADS) {
		return -1;
	}
	// create new stack for thread
	void *stack = (int *)malloc(THREAD_STACK_SIZE);
	// assign top of stack
	void *sp = stack + THREAD_STACK_SIZE - 8;
	// set the first 8 bits to pthread_exit()
	*(unsigned long *) sp = (unsigned long) &pthread_exit;
	// add TCB to arrary
	TCBA[i] = ptr;
	// set the tid
	TCBA[i]->tid = (pthread_t)i;
	// set the stack pointer
	ptr->stackptr = sp;
	// setjmp() to initialize a jmp_buf with your current thread
	if (sigsetjmp(ptr->myreg,0) == 1) {
		return -1;
	}
	// now do mangle(start_thunk) to replace reg values
	ptr->myreg->__jmpbuf[JB_PC] = ptr_mangle((unsigned long int)start_thunk);
	ptr->myreg->__jmpbuf[JB_R12] = (unsigned long int) start_routine;
	ptr->myreg->__jmpbuf[JB_R13] = (unsigned long int) arg;
	ptr->myreg->__jmpbuf[JB_RSP] = ptr_mangle((long int)ptr->stackptr);
	// set status as READY after initializing
	ptr->status = TS_READY;
	// set *thread on success
	*thread = ptr->tid;
	// add to end of queue to schedule
	struct node *middle_q = create_node(TCBA[i]);
	addnode(middle_q);
	return 0;
}

void pthread_exit(void *value_ptr)
{
	ualarm(0,0);
	// set current thread as exited
	head->ptr_t->status = TS_EXITED;
	// call schedule for next thread
	if (head->ptr_t->tid == 0) {
		while (head != NULL) {
			schedule(100);
		}
	}
	if (head != NULL)
		schedule(0);
	exit(0);
}

pthread_t pthread_self(void)
{
	// returns current thread's tid
	return current_t->tid;
}

