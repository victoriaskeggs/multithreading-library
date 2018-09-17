/*
* File:   num_cores.c
* Author: vske511
*
*/

#include <stdio.h>
#include <sys/sysinfo.h>

/**
 * Prints the number of cores in the system to the console
 */
int main(int argc, char** argv) {
	int numCores = get_nprocs_conf();
	printf('This machine has %d cores.', numCores);
}
