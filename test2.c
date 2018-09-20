/* 
 * File:   test2.c
 * Author: robert
 */

#include "dispatchQueue.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void test2() {
    sleep(1);
    printf("test2 running\n");
}

/*
 * Checks asynchronous dispatching.
 * The program should finish before the message from the test2 function is printed.
 * 
 */
int main(int argc, char** argv) {
    // create a concurrent dispatch queue
    dispatch_queue_t * concurrent_dispatch_queue;
    task_t *task;
	//printf("Creating a dispatch queue.\n");
    concurrent_dispatch_queue = dispatch_queue_create(CONCURRENT);
	//printf("Dispatch queue created.\n");
	//printf("Creating a task.\n");
    task = task_create(test2, NULL, "test2");
	//printf("Task created.\n");
	//printf("Dispatching async.\n");
    dispatch_async(concurrent_dispatch_queue, task);
    printf("Safely dispatched\n");
	//printf("Destroying the queue.\n");
    dispatch_queue_destroy(concurrent_dispatch_queue);
	//printf("Queue destroyed.\n");
    return EXIT_SUCCESS;
}
