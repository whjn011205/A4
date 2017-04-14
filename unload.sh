#!/bin/bash
module="four"
device="four"
major=61

# do clean
rm -rf /dev/${device} #remove stale nodes
rmmod ./$module.ko
make clean
