#!/usr/bin/env python3

from __future__ import print_function
import argparse
import os, sys
sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from vm import VMManager
from testLibrary import TestLib
import sched, time
import subprocess

VM_PREFIX="aos"



if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    parser.add_argument("-m","--machine",action="store_true",help="outputs a machine parseable format")
    parser.add_argument("-t","--test",type=str,help="test case file")
    args = parser.parse_args()
    machineParseable = args.machine
    test = args.test

    
    s = sched.scheduler(time.time, time.sleep)
    manager = VMManager()
    vmlist = manager.getRunningVMNames(VM_PREFIX)
    vmobjlist = [manager.getVmObject(name) for name in vmlist]   
    
    for vm in vmobjlist:
        vm.setMemoryStatsPeriod(1)   
        
        print(type(vm))
        print(vm.memoryStats())
