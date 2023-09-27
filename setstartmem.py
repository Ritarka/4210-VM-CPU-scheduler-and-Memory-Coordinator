#!/usr/bin/env python

from __future__ import print_function
from vm import VMManager

MAX_MEMORY = 512
VM_PREFIX = "aos"

if __name__ == '__main__':
    manager = VMManager()
    manager.setAllVmsMaxMemoryWithFilter(VM_PREFIX,MAX_MEMORY)
