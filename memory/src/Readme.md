
## Project Instruction Updates:

1. Complete the function MemoryScheduler() in memory_coordinator.c
2. If you are adding extra files, make sure to modify Makefile accordingly.
3. Compile the code using the command `make all`
4. You can run the code by `./memory_coordinator <interval>`

### Notes:

1. Make sure that the VMs and the host has sufficient memory after any release of memory.
2. Memory should be released gradually. For example, if VM has 300 MB of memory, do not release 200 MB of memory at one-go.
3. Domain should not release memory if it has less than or equal to 100MB of unused memory.
4. Host should not release memory if it has less than or equal to 200MB of unused memory.
5. While submitting, write your algorithm and logic in this Readme.

With the memory co-ordinator, at the beginning and whenever we change the allocated memory of a vm, we first obtain a baseline measurement of their unused memory usage. If the vcpu's unused memory goes 100 below the baseline, we increase their allocated memory and in the next iteration, we take a baseline. Similarly, if the vcpu's unused memory goes 100 over the baseline, we reduce their memory and in the next iteration, we take another baseline. This way of establishing a baseline allows us to ensure we do not starve/overfeed a VM, allowing us to monitor it's own behavior almost independent of our actions to their memory