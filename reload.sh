#!/bin/bash
module="four"
device="four"
major=61


# do clean
rm -rf /dev/${device} #remove stale nodes
rmmod ./$module.ko
make clean

# Compile and load the device
make
rm -rf /dev/${device}	# remove stale devices
mknod /dev/${device} c $major 0
insmod ./$module.ko


