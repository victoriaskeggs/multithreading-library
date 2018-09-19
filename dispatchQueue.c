/*
* File:   dispatchQueue.c
* Author: vske511
*
*/

#include <stdio.h>
#include "dispatchQueue.h"
#include "num_cores.c"
#include <stdlib.h>
#include <string.h>

#define ERROR_STATUS 1
#define P_SHARED 0 // semaphores are shared between different threads on the same process

// TODO memory checks for all mallocs

/**
 * Creates a dispatch queue, setting up any associated threads and a linked list to be used by
 * the added tasks. The queueType is either CONCURRENT or SERIAL.
 * Returns: A pointer to the created dispatch queue.
 */
dispatch_queue_t *dispatch_queue_create(queue_type_t queueType) {

	// Allocate memory to the queue
	dispatch_queue_t *queue = malloc(sizeof(dispatch_queue_t));

	// Check memory was successfully allocated
	if (queue == NULL) {
		printf("Not enough memory available to create a queue.");
		exit(ERROR_STATUS);
	}

	// Set the type of the queue (SERIAL or CONCURRENT)
	queue->queue_type = &queueType;

	// Create a semaphore for the queue and lock the queue
	sem_init(&(queue->queue_semaphore), P_SHARED, 0);

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

	// No tasks have been added to the queue
	queue->first_task = NULL;

	// Allocate memory to the thread pool
	queue->thread_pool = malloc(numThreads * sizeof(dispatch_queue_thread_t));

	// Check memory was successfully allocated
	if (queue->thread_pool == NULL) {
		printf("Not enough memory available to create a thread pool for this queue.");
		exit(ERROR_STATUS);
	}

	// TODO Set up the task allocator thread

	// Add threads to the thread pool
	for (int i = 0; i < numThreads; i++) {

		// Create a new thread type
		dispatch_queue_thread_t *thread;
		thread->queue = queue;

		// Create a semaphore for the thread - no tasks allocated
		sem_init(&(thread->thread_semaphore), P_SHARED, 0);

		// Add the thread type to the pool
		queue->thread_pool[i] = *thread;

		// Start the thread dispatching tasks off the end of the queue
		if (pthread_create(&(thread->thread), NULL, execute_tasks, NULL)) {
			printf("Error creating thread\n");
			exit(ERROR_STATUS);
		}
	}

	// Unlock the queue
	sem_post(&(queue->queue_semaphore));

	return queue;

}

/*
 * Pulls tasks off a queue and allocates them to threads in the queue's thread pool
 */
void allocate_tasks() {

	while (true) {

		// Wait for a task to be added to the task list
	}
}

void execute_tasks() {

	while (true) {

		// Wait on the thread semaphore for a task to become available

		// Execute the task

		// Delete the task
	}
}

/*
 * Destroys the dispatch queue queue. All allocated memory and resources such as semaphores are
 * released and returned.
 */
void dispatch_queue_destroy(dispatch_queue_t *queue) {

	// Wait for the queue to become available
	sem_wait(&(queue->queue_semaphore));

	// Destroy the queue's semaphore
	sem_destroy(&(queue->queue_semaphore));

	// For every thread in the thread pool
	for (int i = 0; i < queue->num_threads; i++) {
		dispatch_queue_thread_t thread = queue->thread_pool[i];

		// Wait for the thread to have no tasks left on it
		//sem_wait(&thread.thread_semaphore);

		// Destroy the thread's semaphore
		sem_destroy(&thread.thread_semaphore);
	}

	// Free the memory allocated to the thread pool
	free(queue->thread_pool);

	// Free the memory allocated to the queue
	free(queue);
}

/*
 * Creates a task. work is the function to be called when the task is executed, param is a pointer to
 * either a structure which holds all of the parameters for the work function to execute with or a single
 * parameter which the work function uses. If it is a single parameter it must either be a pointer or
 * something which can be cast to or from a pointer. The name is a string of up to 63 characters. This
 * is useful for debugging purposes.
 * Returns: A pointer to the created task.
 */
task_t *task_create(void(*work)(void *), void *param, char* name) {

	// Create a task
	task_t *thisTask = malloc(sizeof(task_t));

	// Check memory was successfully allocated
	if (thisTask == NULL) {
		printf("Not enough memory available to create a task.");
		exit(ERROR_STATUS);
	}

	// Set the name of the task
	strcpy(thisTask->name, name);

	// Set the work for the task
	thisTask->work = work;

	// Set the parameters for the work
	thisTask->params = param;

	return thisTask;
}

/*
 * Destroys the task. Call this function as soon as a task has completed. All memory allocated to the
 * task should be returned.
 */
void task_destroy(task_t *task) {

	// Remove the task from the queue when the queue is free

	// Signal the queue is free

	// Free memory allocated to the task
	free(task);
}

/*
 * Sends the task to the queue (which could be either CONCURRENT or SERIAL). This function does
 * not return to the calling thread until the task has been completed.
 */
void dispatch_sync(dispatch_queue_t *queue, task_t *task) {
	// TODO
}

/*
 * Sends the task to the queue (which could be either CONCURRENT or SERIAL). This function
 * returns immediately, the task will be dispatched sometime in the future.
 */
void dispatch_async(dispatch_queue_t *queue, task_t *task) {
	
	// Wait for the queue to become available
	sem_wait(&(queue->queue_semaphore));

	// Find the end of the task queue
	node_t *currentTask = queue->first_task;
	while (currentTask != NULL) {
		currentTask = currentTask->next;
	}

	// Allocate memory to the end of the queue
	currentTask = malloc(sizeof(node_t));

	// Check memory was successfully allocated
	if (currentTask == NULL) {
		printf("Not enough memory available to add the task to the queue.");
		exit(ERROR_STATUS);
	}

	// Add the task to the end of the queue
	currentTask->item = task;

	// Signal the allocator thread that a task is available to be allocated

	// Signal that the queue is available again
	sem_post(&(queue->queue_semaphore));
}