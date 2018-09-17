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
	// Bash command to retrieve number of physical cores
	char command[] = "grep ^cpu\\\\scores /proc/cpuinfo | uniq | awk '{print $4}'";

	// Retrieve output of command to a file
	FILE* commandFile = popen(command, "r");

	// Print output of command to console
	int numCores;
	fscanf(commandFile, "%d", &numCores);
	printf("This machine has %d cores.", numCores);
}
