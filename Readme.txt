This implementation includes a UNIX shell written in C, which uses the process simple_scheduler to schedule processes using Round Robin algorithm.

IMPLEMENTATION : 
	> The scheduler takes two command-line arguments - NCPU and TSLICE.
	> daemonize() function ensures that scheduler runs in the background as daemon process.
	> scheduler uses shared memory to store the child processes.
	> size of shared memory is defined in "SHARED_MEMORY_SIZE".
	> jobs are enqueued into and dequeued from *sharedQueue in shared memory.
	> scheduleProcesses(...) function schedules the jobs as per round robin scheduling policy.
	> history and all other info of the jobs is also stored in the shared memory.

CONTRIBUTION :
	Aditi Sharma(2022025) - coding and debugging
	Ananya Garg(2022068) - coding and debugging
	
GITHUB REPOSITORY LINK:
	https://github.com/Aditi1409sharma/CSE231_Assignment_3.git
