#!/bin/bash
module="4MB"
device="4MB"
major=61

# do clean
rm -rf /dev/${device} #remove stale nodes
rmmod ./$module.ko
make clean
