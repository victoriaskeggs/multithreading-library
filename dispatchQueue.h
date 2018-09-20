/* 
 * File:   dispatchQueue.h
 * Author: robert
 *
 * Modified by: vske511
 */

#ifndef DISPATCHQUEUE_H
#define	DISPATCHQUEUE_H

#include <pthread.h>
#include <semaphore.h>
    
#define error_exit(MESSAGE)     perror(MESSAGE), exit(EXIT_FAILURE)

    typedef enum { // whether dispatching a task synchronously or asynchronously
        ASYNC, SYNC
    } task_dispatch_type_t;
    
    typedef enum { // The type of dispatch queue.
        CONCURRENT, SERIAL
    } queue_type_t;

    typedef struct task {
        char name[64];              // to identify it when debugging
        void (*work)(void *);       // the function to perform
        void *params;               // parameters to pass to the function
        task_dispatch_type_t type;  // asynchronous or synchronous
	sem_t task_sem;		    // for waiting for the task to complete
    } task_t;

	typedef struct node {	// to create linked lists of tasks
		task_t *item;
		struct node *next;
	} node_t;
    
    typedef struct dispatch_queue_t dispatch_queue_t; // the dispatch queue type
    typedef struct dispatch_queue_thread_t dispatch_queue_thread_t; // the dispatch queue thread type

    struct dispatch_queue_thread_t {
        dispatch_queue_t *queue;// the queue this thread is associated with
        pthread_t thread;       // the thread which runs the task
        task_t *task;           // the current task for this thread
    };

    struct dispatch_queue_t {
        queue_type_t *queue_type;				// the type of queue - serial or concurrent
		dispatch_queue_thread_t **thread_pool;	// a dynamically sized thread pool to execute the tasks
		int num_threads;						// the number of threads in the thread pool
		node_t *first_task;						// linked list of tasks
		sem_t queue_lock;						// for locking the queue
		sem_t thread_semaphore;					// the semaphore the threads wait on until a task is allocated
    };
    
    task_t *task_create(void (*)(void *), void *, char*);
    
    void task_destroy(task_t *);

    dispatch_queue_t *dispatch_queue_create(queue_type_t);
    
    void dispatch_queue_destroy(dispatch_queue_t *);
    
    void dispatch_async(dispatch_queue_t *, task_t *);
    
    void dispatch_sync(dispatch_queue_t *, task_t *);
    
    void dispatch_for(dispatch_queue_t *, long, void (*)(long));
    
    int dispatch_queue_wait(dispatch_queue_t *);

#endif	/* DISPATCHQUEUE_H */
