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
	sem_init(&(queue->queue_lock), P_SHARED, 0);

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

	// Create a semaphore for the threads to wait on - no tasks allocated
	sem_init(&(queue->thread_semaphore), P_SHARED, 0);

	// Add threads to the thread pool
	for (int i = 0; i < numThreads; i++) {

		// Create a new thread type
		dispatch_queue_thread_t *thread;
		thread->queue = queue;

		// Add the thread type to the pool
		queue->thread_pool[i] = *thread;

		// Start the thread dispatching tasks off the end of the queue
		if (pthread_create(&(thread->thread), NULL, execute_tasks, &thread)) {
			printf("Error creating thread\n");
			exit(ERROR_STATUS);
		}
	}

	// Unlock the queue
	sem_post(&(queue->queue_lock));

	return queue;

}

void execute_tasks(dispatch_queue_thread_t *thread) {

	while (true) {

		// Wait on the thread semaphore for a task to become available
		sem_wait(&(thread->queue->thread_semaphore));

		// Wait for the queue to become available
		sem_wait(&(thread->queue->queue_lock));

		// Grab the first task off the queue
		task_t *task = &thread->queue->first_task->item;

		// Take the task out of the queue
		thread->queue->first_task = &thread->queue->first_task->next;

		// Release the queue lock
		sem_post(&(thread->queue->queue_lock));

		// Update the current task
		thread->task = &task;

		// Execute the task
		task->work(&task->params);

		// Destroy the task
		task_destroy(&task);

	}
}

/*
 * Destroys the dispatch queue queue. All allocated memory and resources such as semaphores are
 * released and returned.
 */
void dispatch_queue_destroy(dispatch_queue_t *queue) {

	// Wait for the queue to become available
	sem_wait(&(queue->queue_lock));

	// Destroy the queue's lock
	sem_destroy(&(queue->queue_lock));

	// Destroy the thread semaphore
	sem_destroy(&(queue->thread_semaphore));

	// For every thread in the thread pool
	for (int i = 0; i < queue->num_threads; i++) {
		dispatch_queue_thread_t thread = queue->thread_pool[i];

		// TODO deallocate the memory?
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

	// Set task as async
	task->type = ASYNC;

	// Allocate memory to new task type
	node_t *newTask = malloc(sizeof(node_t));

	// Check memory was successfully allocated
	if (newTask == NULL) {
		printf("Not enough memory available to add the task to the queue.");
		exit(ERROR_STATUS);
	}

	// Add the task to the task type
	newTask->item = task;

	// Wait for the dispatch queue to become available
	sem_wait(&(queue->queue_lock));

	// Find the end of the task queue
	node_t *currentTask = queue->first_task;
	while (currentTask != NULL) {
		currentTask = currentTask->next;
	}

	// Add the new task
	currentTask = newTask;

	// Unlock the queue
	sem_post(&(queue->queue_lock));

	// Signal the threads that a new task is available
	sem_post(&(queue->thread_semaphore));
}