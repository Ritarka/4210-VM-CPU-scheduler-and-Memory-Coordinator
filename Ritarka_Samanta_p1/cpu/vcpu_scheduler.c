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

int is_exit = 0; // DO NOT MODIFY THIS VARIABLE


void CPUScheduler(virConnectPtr conn,int interval);

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

	// Get the total number of pCpus in the host
	signal(SIGINT, signal_callback_handler);

	while(!is_exit)
	// Run the CpuScheduler function that checks the CPU Usage and sets the pin at an interval of "interval" seconds
	{
		CPUScheduler(conn, interval);
		sleep(interval);
	}

	// Closing the connection
	virConnectClose(conn);
	return 0;
}

double* time;

typedef struct _data_t {
	int id;
	double use;
} data_t;

data_t assign[4];

double stats[4][3];

/* COMPLETE THE IMPLEMENTATION */
void CPUScheduler(virConnectPtr conn, int interval)
{
	virDomainPtr* domains;

	unsigned int flags = VIR_CONNECT_LIST_DOMAINS_RUNNING |
                     VIR_CONNECT_LIST_DOMAINS_PERSISTENT;

	int num_domains = virConnectListAllDomains(conn, &domains, flags);

	int total_vcpus = 0;
	for (int i = 0; i < num_domains; i++) {
		total_vcpus += virDomainGetVcpusFlags(domains[i], VIR_DOMAIN_VCPU_MAXIMUM);
	}

	static int initialized = 0;

	if (!initialized) {
		time = (double*)calloc(total_vcpus, sizeof(double));
		initialized = 1;
		memset(stats, 0, sizeof(unsigned long long) * 4 * 3);
	}

	double diff[total_vcpus];

	for (int i = 0; i < num_domains; i++) {
		virDomainInfoPtr info = (virDomainInfoPtr)malloc(sizeof(virDomainInfo));
		virDomainGetInfo(domains[i], info);

		virVcpuInfoPtr cpu_info = (virVcpuInfoPtr)malloc(sizeof(virVcpuInfo));
		
		virDomainGetVcpus(domains[i], cpu_info, 1, NULL, 0);

		// printf("v_num: %d,  cpu_time: %lld p_num: %d num: %d\n", i, cpu_info->cpuTime, cpu_info->cpu, cpu_info->number);
		// printf("Misc stats: [%i] Num_cpus: %d cpu_time: %llu  state:%d\n", i, info->nrVirtCpu, info->cpuTime, info->state);

		double cpu_time = (double)info->cpuTime / 1e9; // Convert nanoseconds to seconds

		// Calculate CPU usage as a percentage
		diff[i] = cpu_time - time[i];  
		time[i] = cpu_time;


		// u_int8_t map = 2;
		// int ret = virDomainPinVcpu(domains[i], 0, &map, 1);
		// if (ret != 0)
		// 	printf("Remapping cpu failed\n");

		free(info);
		free(cpu_info);
	}


	double total = 0;
	for (int i = 0; i < total_vcpus; i++)
		total += diff[i];

	double rate[total_vcpus];
	double load[4] = {0, 0, 0, 0};
	for (int i = 0; i < num_domains; i++) {
		// printf("CPU Utilization: %.2lf%%\n", diff[i] * 100 / total);
		rate[i] = diff[i] * 100 / total;

		virVcpuInfoPtr cpu_info = (virVcpuInfoPtr)malloc(sizeof(virVcpuInfo));
		virDomainGetVcpus(domains[i], cpu_info, 1, NULL, 0);
		load[cpu_info->cpu] += rate[i];
	}

	double variation = 0;
	for (int i = 0; i < 4; i++) {
		// printf("Load [%d] %lf\n", i, load[i]);
		variation += abs(25 - load[i]);
	}


	double avg_usage = 0;
	
	for (int i = 0; i < 4; i++) {
		int nparams = 0;
		virNodeCPUStatsPtr params;
		if (virNodeGetCPUStats(conn, i, NULL, &nparams, 0) == 0 && nparams != 0) {
			if ((params = malloc(sizeof(virNodeCPUStats) * nparams)) == NULL)
				return;
			memset(params, 0, sizeof(virNodeCPUStats) * nparams);
			if (virNodeGetCPUStats(conn, i, params, &nparams, 0))
				return;
		}

		double kernel, user, idle = 0;
		for (int i = 0; i < nparams; i++) {
			if (!strcmp(params[i].field, "kernel"))
				kernel = params[i].value / 1e9;
			else if (!strcmp(params[i].field, "user"))
				user = params[i].value / 1e9;
			else if (!strcmp(params[i].field, "idle")) {
				idle = params[i].value / 1e9;
				// printf("Here\n");
			}

		}

		
		double c_user, c_kernel, c_idle = 0;
		c_user = user - stats[i][0];
		c_kernel = kernel - stats[i][1];
		c_idle = idle - stats[i][2];
		
		stats[i][0] = user;
		stats[i][1] = kernel;
		stats[i][2] = idle;

		// printf("user: %lf kernel: %lf idle: %lf\n", c_user, c_kernel, c_idle);

		printf("Another Measure> %lf\n", (double)(c_user)/(c_user+c_idle));
		avg_usage += (double)(c_user)/(c_user+c_idle);


		// printf("Usage?: %lf\n", (double)(user)/(user+idle));
	
		free(params);
	}


	if ((avg_usage / 4) < 0.10f) return;
	if (variation < 15) return;

	data_t info[total_vcpus];
	for (int i = 0; i < total_vcpus; i++) {
		info[i].id = i;
		info[i].use = rate[i];
	}

	//bubble sort, cause I can't be bothered to do better
	for (int i = 0; i < total_vcpus; i++) {
        for (int j = 0; j < total_vcpus - i - 1; j++) {
            if (info[j].use > info[j + 1].use) {
                data_t temp = info[j];
                info[j] = info[j + 1];
                info[j + 1] = temp;
            }
        }
    }

	for (int i = 0; i < num_domains; i++) {
		unsigned char pcpu = 1 << (i % 4);
		virDomainPinVcpu(domains[info[i].id], 0, &pcpu, 1);
		printf("Moving domain %d to %d\n", i, pcpu);
	}

}




