## Project Instruction Updates:

1. Complete the function CPUScheduler() in vcpu_scheduler.c
2. If you are adding extra files, make sure to modify Makefile accordingly.
3. Compile the code using the command `make all`
4. You can run the code by `./vcpu_scheduler <interval>`
5. While submitting, write your algorithm and logic in this Readme.


First I obtain the cpu time for each vcpu. This way, I can see which vcpus are heavily used, as a percentage of the total. Then I calculate how much, as a percent, each pcpu is beuing utilized. Once we get that value, I calculate the variation between the pcpus by summing up the deviation from the average (25% in this case since we have 4 pcpus). The the 'variance' exceed a hardcoded number (25), we choose to re-pin the vcpus. To repin, we sort the vcpus by their percent usage and assign the heaviest to the 1st pcpu, the 2nd heaviest to the 2nd vcpu, and so on, wrapping back to the top when the number of vcpus exceed the number of pcpus.