/*
* File:   dispatchQueue.c
* Author: vske511
*
*/

#include <stdio.h>
#include "dispatchQueue.h"
#include <stdlib.h>
#include <string.h>

#define ERROR_STATUS 1
#define P_SHARED 0 // semaphores are shared between different threads on the same process

// Helper methods
void* execute_tasks(void *thread);
void dispatch(dispatch_queue_t *queue, task_t *task, task_dispatch_type_t type);

/*
* Finds the number of physical cores on the device.
*/
int getNumCores() {
	// Bash command to retrieve number of physical cores
	char command[] = "grep ^cpu\\\\scores /proc/cpuinfo | uniq | awk '{print $4}'";

	// Read number of cores into a file
	FILE* commandFile = popen(command, "r");

	// Retrieve number of cores
	int numCores;
	fscanf(commandFile, "%d", &numCores);

	return numCores;
}

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
		printf("Not enough memory available to create a queue.\n");
		exit(ERROR_STATUS);
	}

	// Set the type of the queue (SERIAL or CONCURRENT)
	queue->queue_type = &queueType;

	// Create a semaphore for the queue and lock the queue
	if (sem_init(&(queue->queue_lock), P_SHARED, 0) != 0) {
		printf("Unable to create a lock semaphore for the queue.\n");
		exit(ERROR_STATUS);
	}

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

	// No threads are currently executing tasks
	queue->num_threads_executing = 0;

	// Allocate memory to the thread pool
	queue->thread_pool = malloc(numThreads * sizeof(dispatch_queue_thread_t*));

	// Check memory was successfully allocated
	if (queue->thread_pool == NULL) {
		printf("Not enough memory available to create a thread pool for this queue.");
		exit(ERROR_STATUS);
	}

	// Create a semaphore for the threads to wait on - no tasks allocated
	if (sem_init(&(queue->thread_semaphore), P_SHARED, 0) != 0) {
		printf("Unable to a semaphore for threads to wait on.\n");
		exit(ERROR_STATUS);
	}

	// Add threads to the thread pool
	for (int i = 0; i < numThreads; i++) {

		// Create a new thread type and allocate memory
		dispatch_queue_thread_t *thread = malloc(sizeof(dispatch_queue_thread_t));

		// Check memory was successfully allocated
		if (thread == NULL) {
			printf("Not enough memory available to create a thread for this queue.");
			exit(ERROR_STATUS);
		}

		thread->queue = queue;

		// Add the thread type to the pool
		queue->thread_pool[i] = thread;

		// Start the thread dispatching tasks off the end of the queue
		if (pthread_create(&(thread->thread), NULL, execute_tasks, thread)) {
			printf("Error creating thread\n");
			exit(ERROR_STATUS);
		}

	}

	// Unlock the queue
	sem_post(&(queue->queue_lock));

	return queue;

}

/*
 * Continously pulls tasks off the dispatch queue *threadUncast points to and executes them. Tasks are 
 * destroyed after they are completed.
 *
 */
void *execute_tasks(void *threadUncast) {

	// Cast the thread
	dispatch_queue_thread_t *thread = (dispatch_queue_thread_t*)threadUncast;

	while (1) {

		// Wait on the thread semaphore for a task to become available
		sem_wait(&(thread->queue->thread_semaphore));

		// Wait for the queue to become available
		sem_wait(&(thread->queue->queue_lock));

		// Grab the first task off the queue
		task_t *task = thread->queue->first_task->item;

		// One more thread is executing
		thread->queue->num_threads_executing++;

		// Take the task out of the queue
		thread->queue->first_task = thread->queue->first_task->next;

		// Release the queue lock
		sem_post(&(thread->queue->queue_lock));

		// Update the current task
		thread->task = task;

		// Execute the task
		task->work(task->params);

		// Wait for the queue to become available
		sem_wait(&(thread->queue->queue_lock));

		// The thread is no longer executing
		thread->queue->num_threads_executing--;

		// Release the queue lock
		sem_post(&(thread->queue->queue_lock));

		// Indicate the task is complete
		sem_post(&(task->task_sem));

		// Destroy the task
		task_destroy(task);

	}
}

/*
 * Destroys the dispatch queue queue. All allocated memory and resources such as semaphores are
 * released and returned.
 */
void dispatch_queue_destroy(dispatch_queue_t *queue) {

	// Wait for the queue to become available
	sem_wait(&(queue->queue_lock));

	// For every thread in the thread pool
	for (int i = 0; i < queue->num_threads; i++) {
		dispatch_queue_thread_t *thread = queue->thread_pool[i];

		// Cancel the thread
		pthread_cancel(thread->thread);

		// Free the memory assigned to the thread
		free(thread);
	}

	// Free any tasks still on the queue
	if (queue->first_task != NULL) {
		node_t *currentNode = queue->first_task;
		while (currentNode->next != NULL) {
			currentNode = currentNode->next;
			task_destroy(currentNode->item);
			free(currentNode);
		}
	}

	// Destroy the queue's lock
	sem_destroy(&(queue->queue_lock));

	// Destroy the thread semaphore
	sem_destroy(&(queue->thread_semaphore));

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

	// Initialise a semaphore for indicating when a task has completed
	if (sem_init(&(thisTask->task_sem), P_SHARED, 0) != 0) {
		printf("Unable to create a semaphore to indicate a task has completed.\n");
		exit(ERROR_STATUS);
	}

	return thisTask;
}

/*
 * Destroys the task. Call this function as soon as a task has completed. All memory allocated to the
 * task should be returned.
 */
void task_destroy(task_t *task) {

	// Destroy the semaphore on the task
	sem_destroy(&(task->task_sem));

	// Free memory allocated to the task
	free(task);
}

/*
 * Sends the task to the queue (which could be either CONCURRENT or SERIAL). This function does
 * not return to the calling thread until the task has been completed.
 */
void dispatch_sync(dispatch_queue_t *queue, task_t *task) {

	// Dispatch the task
	dispatch(queue, task, SYNC);

	// Wait for the task to complete
	sem_wait(&(task->task_sem));
}

/*
 * Sends the task to the queue (which could be either CONCURRENT or SERIAL). This function
 * returns immediately, the task will be dispatched sometime in the future.
 */
void dispatch_async(dispatch_queue_t *queue, task_t *task) {

	// Dispatch the task
	dispatch(queue, task, ASYNC);
}

/*
 * Sends the task to the queue (which could be either CONCURRENT or SERIAL). This function
 * returns immediately, the task will be dispatched sometime in the future. Sets the task
 * to type.
 */
void dispatch(dispatch_queue_t *queue, task_t *task, task_dispatch_type_t type) {

	// Wait for the dispatch queue to become available
	sem_wait(&(queue->queue_lock));

	// Allocate memory to new task type
	node_t* newNode = malloc(sizeof(node_t));

	// Check memory was successfully allocated
	if (newNode == NULL) {
		printf("Not enough memory available to add the task to the queue.\n");
		exit(ERROR_STATUS);
	}

	// Add the task
	newNode->item = task;

	// Set task as async
	newNode->item->type = type;

	// Find the end of the task queue
	if (queue->first_task == NULL) {
		queue->first_task = newNode;
	}
	else {
		node_t *currentNode = queue->first_task;
		while (currentNode->next != NULL) {
			currentNode = currentNode->next;
		}
		currentNode->next = newNode;
	}

	// Unlock the queue
	sem_post(&(queue->queue_lock));

	// Signal the threads that a new task is available
	sem_post(&(queue->thread_semaphore));
}
/*
 * Waits (blocks) until all tasks on the queue have completed. If new tasks are
 * added to the queue after this is called they are ignored.
 */
void dispatch_queue_wait(dispatch_queue_t *queue) {

	// Check the finish condition
	while (queue->num_threads_executing != 0 || queue->first_task != NULL) {}
}

/*
 * Executes   the  work  function  number  of   times   (in   parallel   if   the
 * queue is CONCURRENT).   Each iteration   of   the  work  function   is   passed
 * an   integer   from  0 to number-1.   The  dispatch_for function does not return
 * until all iterations of the work function have completed.
 */
void dispatch_for(dispatch_queue_t *queue, long number, void(*work)(long)) {

	// Dispatch all the tasks asynchronously
	for (long i = 0; i < number; i++) {
		char *name = "dispatch_for_work_function";
		task_t *task = task_create((void(*)(void*))work, (void*)i, name);
		dispatch_async(queue, task);
	}

	// Wait for all the tasks to complete
	dispatch_queue_wait(queue);
}
