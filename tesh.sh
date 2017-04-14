#!/bin/sh

sudo make uninstall
rm -f /dev/fourmb_device

sudo mknod /dev/fourmb_device c 61 0
make
sudo make install


dmesg | tail -10
