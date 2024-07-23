# Basic Threading Library

Implemented threading in the user-space with a round-robin scheduler.

**The working branch is FIXSCHEDULE**


## Description
This library includes the major three functions in a threading library

`pthread_create()`: Creates a new thread by initializing a thread_control_block
```
struct thread_control_block {
	pthread_t tid;             // thread id
	void *stackptr;            // pointer to stack
	jmp_buf myreg;             // register state
	enum thread_status status; // thread status of EXITED, RUNNING, or READY
};
MAX_THREADS: 128
```
`pthread_self`: Returns the thread id of the current running thread
```
Current thread is determined by global variable current_t which is updated everytime a siglongjmp occurs
```

`pthread_exit()`: Called when thread is done executing.
```
Sets thread status to EXITED and calls schedule(), the thread is freed after the next context switch
```

#### Scheduling

Implemented round robin scheduling using a linked list queue

`ualarm`: used to send a SIGALRM signal every interval - SCHEDULER_INTERVAL_USECS: 50 ms 

`sig_handler`: catches the signal and calls schedule()

`scheduler_init()`: initializes the scheduler and schedules main thread

## External Resouces
- `man pages` of various functions ie. siglongjmp, sigsetjmp, sigaction
- Lecture slides and notes taken during class
- Piazza resources and student questions
- Student made test cases: max threads and creating threads mid-execution

## Additional Notes
The most difficulty I had on this project was scheduling the threads. My initial implementation of schedule() included a switch statement within a while loop, but it never did a siglongjmp(), so although the queue was working, only main was ever running. In my second implementation I opted for a simpler function with if statements instead of the case statement so I could break out of the while(1) loop. This allowed me to finally schedule and run all the threads created, but with some problems. For a while main was exiting before the third thread exited because in my implementation, I schedule main first although it was already executing. I do not know what change I made to fix this problem; however, after I cleaned up some of my code the test case was a success. Later, I created a freenode() function to ensure the stack is only freed after a new thread is being run.

Note: I decided to use sigsetjmp and siglongjmp because they are the counterparts to sigaction.
