#include "tls.h"
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <sys/mman.h>
#include <signal.h>
#include <string.h>
#include <stdbool.h>
/*
 * This is a good place to define any data structures you will use in this file.
 * For example:
 *  - struct TLS: may indicate information about a thread's local storage
 *    (which thread, how much storage, where is the storage in memory)
 *  - struct page: May indicate a shareable unit of memory (we specified in
 *    homework prompt that you don't need to offer fine-grain cloning and CoW,
 *    and that page granularity is sufficient). Relevant information for sharing
 *    could be: where is the shared page's data, and how many threads are sharing it
 *  - Some kind of data structure to help find a TLS, searching by thread ID.
 *    E.g., a list of thread IDs and their related TLS structs, or a hash table.
 */

 typedef struct{
	unsigned long address;
	int ref_count;
} page;

typedef struct thread_local_storage{
	pthread_t tid;
	unsigned long size;
	unsigned int page_num;
	page **pages;
} TLS;

typedef struct{
	pthread_t tid;
	TLS *tls;
} tid_tls_pair;

/*
 * Now that data structures are defined, here's a good place to declare any
 * global variables.
 */
static tid_tls_pair *tid_tls_pairs[MAX_THREAD_COUNT];
pthread_mutex_t mutex; // lock
int page_size;

/*
 * With global data declared, this is a good point to start defining your
 * static helper functions.
 */

/*
 * Lastly, here is a good place to add your externally-callable functions.
 */ 

 int tls_find(pthread_t tid) {
	for (int i = 0; i < MAX_THREAD_COUNT; i++) {
		if(tid_tls_pairs[i]->tid == tid) {
			return i;
		}
	}
	return -1;
 }

 int tls_find_next() {
	int ret = -1;
	for (int i = 0; i < MAX_THREAD_COUNT; i++) {
        if (tid_tls_pairs[i]->tid == -1) {
            ret = i;
        }
    }
	return ret;
 }

void tls_handle_page_fault(int sig, siginfo_t *si, void *context) {
	unsigned long p_fault = ((unsigned long) si->si_addr & ~(page_size - 1));
	// find location of segfault to see if in page
	int it = 0;
	while (tid_tls_pairs[it] != NULL) {
		for (int i = 0; i < tid_tls_pairs[it]->tls->page_num; i++) {
			if (tid_tls_pairs[it]->tls->pages[i]->address == p_fault) {
				pthread_exit(NULL);
			}
		}
		it++;
	}
	// call signal
	signal(SIGSEGV, SIG_DFL);
	signal(SIGBUS, SIG_DFL);
	raise(sig);
}

// unprotect pages 0-> read 1-> write
void tls_unprotect(page *p, int prot) {
	if (mprotect((void *) p->address, page_size, prot)) {
 		fprintf(stderr, "tls_unprotect: could not unprotect page\n");
 		exit(1);
 	}
}

// protect pages
void tls_protect(page *p) {
 	if (mprotect((void *) p->address, page_size, PROT_NONE)) {
 		fprintf(stderr, "tls_protect: could not protect page\n");
 		exit(1);
 	}
}

/*
 The tls_create() creates a local storage area (LSA) for the currently-executing thread. This
LSA must hold at least size bytes.
This function returns 0 on success, and -1 for errors. It is an error if a thread already has more
than 0 bytes of local storage.
*/
void tls_init() {
 	struct sigaction sigact;
 	page_size = getpagesize();
 	/* Handle page faults (SIGSEGV, SIGBUS) */
 	sigemptyset(&sigact.sa_mask);
 	/* Give context to handler */
 	sigact.sa_flags = SA_SIGINFO;
 	sigact.sa_sigaction = tls_handle_page_fault;
	sigaction(SIGBUS, &sigact, NULL);
 	sigaction(SIGSEGV, &sigact, NULL);
	// create pair
	tid_tls_pair *pair = malloc(sizeof(tid_tls_pair));
	pair->tid = -1;

	for (int i = 0; i < MAX_THREAD_COUNT; i++) {
		tid_tls_pairs[i] = pair;
	}
}

int tls_create(unsigned int size)
{
	static bool is_first_call = true;
	if (is_first_call) {
		is_first_call = false;
		tls_init();
	}
	pthread_t tid = pthread_self();
	// if size <= 0 throw error
	if (size <= 0) {
		return -1;
	}
	// if LSA exits throw error
	if (tls_find(tid) != -1) {
		return -1;
	}
	// initialize TLS
	TLS *t = malloc(sizeof(TLS));
	t->tid = tid;
	t->size = size;
	t->page_num = (size + page_size - 1) / page_size;
	t->pages = calloc(t->page_num, sizeof(page *));
	// initialize pages
	for (int i = 0; i < t->page_num; i++) {
		page *p = malloc(sizeof(page));
		p->address = (unsigned long)mmap(0, page_size, PROT_NONE, MAP_ANON | MAP_PRIVATE, 0, 0);
		if (p->address == (unsigned long)MAP_FAILED) {
			return -1;
		}
		p->ref_count = 1;
		t->pages[i] = p;
	}
	int j = tls_find_next();
	tid_tls_pairs[j] = calloc(1, sizeof(tid_tls_pair));
	tid_tls_pairs[j]->tls = t;
	tid_tls_pairs[j]->tid = tid;
	return 0;
}

int tls_destroy()
{
	// free all data structures
	// remove node from LL
	pthread_t tid = pthread_self();
	int index = tls_find(tid);
	if (index == -1) {
		return -1;
	}
	TLS *tls = tid_tls_pairs[index]->tls;

	// free tls - and page array
	for (int i = 0; i < tls->page_num; i++) {
		if (tls->pages[i]->ref_count > 1) {
			tls->pages[i]->ref_count--;
		}
		else {
			munmap((void *)tls->pages[i]->address, page_size);
			free(tls->pages[i]);
		}
	}
	free(tls);

	return 0;
}

int tls_read(unsigned int offset, unsigned int length, char *buffer)
{
	pthread_t tid = pthread_self();
	int index = tls_find(tid);
	
	// error check tls_read()
	if (index == -1) {
		return -1;
	}
	TLS *tls= tid_tls_pairs[index]->tls;
	if (offset + length > tls->size) {
		return -1;
	}
	// unprotect the pages
	pthread_mutex_lock(&mutex);
	for (int i = 0; i < tls->page_num; i++) {
			tls_unprotect(tls->pages[i], PROT_READ);
	}
	// read from pages to buffer
	int iterator = offset;
	unsigned int offset_p;
	unsigned int num_p;
	// char *copy;
	while (iterator < (offset + length)) {
		offset_p = iterator % page_size;
		num_p = iterator / page_size;
		page *pcpy = tls->pages[num_p];
		char *charcpy = (char *)pcpy->address + offset_p;
		buffer[iterator] = *charcpy;
		iterator++;
	}
	buffer[iterator] = '\0';
	for (int i = 0; i < tls->page_num; i++) {
			tls_protect(tls->pages[i]);
	}
	pthread_mutex_unlock(&mutex);
	return 0;
}

int tls_write(unsigned int offset, unsigned int length, const char *buffer)
{
	// error check
	pthread_t tid = pthread_self();
	int index = tls_find(tid);
	if (index == -1) {
		return -1;
	}
	TLS *tls = tid_tls_pairs[index]->tls;
	if (offset + length > tls->size) {
		return -1;
	}
	
	// lock on this thread
	pthread_mutex_lock(&mutex);
	// unprotect pages for write
	for (int i = 0; i < tls->page_num; i++) {
			tls_unprotect(tls->pages[i], PROT_WRITE);
	}

	// int cnt = 0;
	for (int cnt = 0, idx = offset; idx < (offset + length); ++cnt, ++idx) {
 	// Calculate page number and offset within
	int page_num_p = offset >> 12;
	int offset_p = idx & (page_size - 1);
 	// that page, from idx and page size.Then:
		if (tls->pages[page_num_p]->ref_count > 1) {
			// create a private copy (COW)
			// create a copy of page if shared
			page *p = malloc(sizeof(page));
			p->ref_count = 1;
			// create new address
			p->address = (unsigned long) mmap(0, page_size, PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0,0);
			memcpy((void *)p->address, (void *)tls->pages[page_num_p]->address, page_size);
			tls->pages[page_num_p] = p;
			tls->pages[page_num_p]->ref_count--;
			tls_protect(tls->pages[page_num_p]);
			tls->pages[page_num_p] = p;
		}
		char *charcpy = (char *)(tls->pages[page_num_p]->address + offset_p);
		memcpy((char *) charcpy, (char *)(buffer + cnt), sizeof(char));
	}

	// protect pages
	for (int i = 0; i < tls->page_num; i++) {
			tls_protect(tls->pages[i]);
	}
	// unlock thread
	pthread_mutex_unlock(&mutex);
	return 0;
}

int tls_clone(pthread_t tid)
{
	pthread_t tid_s = pthread_self();
	// source
	int index_c = tls_find(tid_s);
	int index_t = tls_find(tid);

	// error checck
	if (index_c == -1) {
		return -1;
	}
	if (index_t != -1) {
		return -1;
	}
	// TLS *tls_current = tid_tls_pairs[index_c]->tls;
	TLS *tls_target = tid_tls_pairs[index_t]->tls;

	// copies target LSA to current

	TLS *t = malloc(sizeof(TLS));
	t->size = tls_target->size;
	t->page_num = tls_target->page_num;
	t->tid = tid;
	// allcate pages
	for (int i = 0; i < t->page_num; i++) {
		tls_unprotect(t->pages[i], PROT_READ);
		memcpy(t->pages[i],tls_target->pages[i], sizeof(page));
		tls_protect(t->pages[i]);
		t->pages[i]->ref_count++;
	}
	int n = tls_find_next();
	tid_tls_pairs[n]->tid = tid;
	tid_tls_pairs[n]->tls = t;
	return 0;
}
