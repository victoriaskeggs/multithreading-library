/*
* File:   num_cores.c
* Author: vske511
*
*/

#include <stdio.h>
#include <sys/sysinfo.h>

/*
 * Prints the number of physical cores on the device to the console.
 */
int main(int argc, char** argv) {
	
	// Print number of cores to the console
	int numCores = getNumCores();
	printf("This machine has %d cores.", numCores);
}

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
