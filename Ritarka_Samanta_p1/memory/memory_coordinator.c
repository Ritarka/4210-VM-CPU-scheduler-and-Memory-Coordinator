#include<stdio.h>
#include<stdlib.h>
#include<libvirt/libvirt.h>
#include<math.h>
#include<string.h>
#include<unistd.h>
#include<limits.h>
#include<signal.h>
#define MIN(a,b) ((a)<(b)?a:b)
#define MAX(a,b) ((a)>(b)?a:b)

int is_exit = 0; // DO NOT MODIFY THE VARIABLE

void MemoryScheduler(virConnectPtr conn,int interval);

/*
DO NOT CHANGE THE FOLLOWING FUNCTION
*/
void signal_callback_handler()
{
	printf("Caught Signal");
	is_exit = 1;
}

/*
DO NOT CHANGE THE FOLLOWING FUNCTION
*/
int main(int argc, char *argv[])
{
	virConnectPtr conn;

	if(argc != 2)
	{
		printf("Incorrect number of arguments\n");
		return 0;
	}

	// Gets the interval passes as a command line argument and sets it as the STATS_PERIOD for collection of balloon memory statistics of the domains
	int interval = atoi(argv[1]);
	
	conn = virConnectOpen("qemu:///system");
	if(conn == NULL)
	{
		fprintf(stderr, "Failed to open connection\n");
		return 1;
	}

	signal(SIGINT, signal_callback_handler);

	while(!is_exit)
	{
		// Calls the MemoryScheduler function after every 'interval' seconds
		MemoryScheduler(conn, interval);
		sleep(interval);
	}

	// Close the connection
	virConnectClose(conn);
	return 0;
}

/*
COMPLETE THE IMPLEMENTATION
*/
void MemoryScheduler(virConnectPtr conn, int interval)
{
	virDomainPtr* domains;

	unsigned int conn_flags = VIR_CONNECT_LIST_DOMAINS_RUNNING;

	int num_domains = virConnectListAllDomains(conn, &domains, conn_flags);

	static int first = 1;
	static double* unused_arr;
	static int* baseline;

	if (first) {
		first = 0;
		unused_arr = (double*)calloc(num_domains, sizeof(double));
		baseline = (int*)calloc(num_domains, sizeof(int));
	}


	for (int i = 0; i < num_domains; i++) {
		virDomainSetMemoryStatsPeriod(domains[i], interval, VIR_DOMAIN_AFFECT_LIVE);

		virDomainMemoryStatPtr stats = (virDomainMemoryStatPtr)calloc(VIR_DOMAIN_MEMORY_STAT_NR, sizeof(virDomainMemoryStatStruct));

		virDomainMemoryStats(domains[i], stats, 10, 0);

		double usuable, available, unused, balloon, rss = 0;
		for (int j = 0; j < VIR_DOMAIN_MEMORY_STAT_NR; j++) {
			// printf("%d, %d %lld\n", i, stats[i].tag, stats[i].val);
			if (stats[j].tag == VIR_DOMAIN_MEMORY_STAT_UNUSED)
				unused = stats[j].val / (double)1024;
			else if (stats[j].tag == VIR_DOMAIN_MEMORY_STAT_AVAILABLE)
				available = stats[j].val / (double)1024;
			else if (stats[j].tag == VIR_DOMAIN_MEMORY_STAT_USABLE)
				usuable = stats[j].val / (double)1024;
			else if (stats[j].tag == VIR_DOMAIN_MEMORY_STAT_ACTUAL_BALLOON)
				balloon = stats[j].val / (double)1024;
			else if (stats[j].tag == VIR_DOMAIN_MEMORY_STAT_RSS)
				rss = stats[j].val / (double)1024;
		}

		// printf("[%d] Unused: %.2f available: %.2f usable: %.2f balloon: %.2f rss: %.2f\n", i, unused, available, usuable, balloon, rss);
		// printf("[%d] Total: %.2lf\n", i, unused + available + usuable + balloon + rss);

		printf("[%d] Actual [%.2f], Unused: [%.2f]\n", i, balloon, unused);

		// unsigned long max = virDomainGetMaxMemory(domains[i]) >> 10;
		// printf("Max: %ld\n", max);

		if (!baseline[i]) {
			unused_arr[i] = unused;
			baseline[i] = 1;
		}

		int new_val = unused;

		// // we must shrink
		if (new_val - unused_arr[i] > 100) {
			unsigned long new_size = balloon - 100;
			if (new_size < 200)
				new_size = 200;

			virDomainSetMemory(domains[i], new_size << 10);
			printf("\t Shrink new memory: %ld\n", new_size);
			baseline[i] = new_val;
		}

		// // we must grow
		if (unused_arr[i] - new_val > 100) {
			unsigned long new_size = balloon + 100;
			if (new_size > 2048)
				new_size = 2048;

			virDomainSetMemory(domains[i], new_size << 10);
			printf("\t Grow new memory: %ld\n", new_size);
			baseline[i] = new_val;
		}


		free(stats);
	}

	return ;
}
