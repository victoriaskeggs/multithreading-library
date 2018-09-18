/*
* File:   dispatchQueue.c
* Author: vske511
*
*/

#include <stdio.h>
#include "dispatchQueue.h"
#include "num_cores.c"
#include <stdlib.h>

#define ERROR_STATUS 1

/**
 * Creates a dispatch queue, setting up any associated threads and a linked list to be used by
 * the added tasks. The queueType is either CONCURRENT or SERIAL.
 * Returns: A pointer to the created dispatch queue.
 */
dispatch_queue_t *dispatch_queue_create(queue_type_t queueType) {

	// Define the queue and queue type
	dispatch_queue_t *queue;
	queue->queue_type = &queueType;

	// Find the number of threads that the thread pool should contain. An async queue should contain one
	// thread and a sync queue should contain the same number of threads as there are physical cores.
	int numThreads; 
	
	if (*(queue->queue_type) == SERIAL) {
		numThreads = 1;
	}
	else {
		numThreads = getNumCores();
	}
	queue->num_threads = numThreads;

	// Allocate memory to the thread pool
	queue->thread_pool = malloc(numThreads * sizeof(dispatch_queue_thread_t));

	// Check memory was successfully allocated
	if (queue->thread_pool == NULL) {
		printf("Not enough memory available to create a thread pool for this queue.");
		exit(ERROR_STATUS);
	}

	// Add threads to the thread pool
	for (int i = 0; i < numThreads; i++) {

		// Create a new thread
		dispatch_queue_thread_t thread;
		thread.queue = queue;

		// Add the thread to the pool
		queue->thread_pool[i] = thread;
	}

	return queue;

}

/*
 * Destroys the dispatch queue queue. All allocated memory and resources such as semaphores are
 * released and returned.
 */
void dispatch_queue_destroy(dispatch_queue_t *queue) {

	// Free any semaphores assigned to threads
	for (int i = 0; i < queue->num_threads; i++) {
		dispatch_queue_thread_t thread = queue->thread_pool[i];
		sem_destroy(&thread.thread_semaphore);
	}

	// Free the memory allocated to the thread pool
	free(queue->thread_pool);
}
