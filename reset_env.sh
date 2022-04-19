#!/bin/bash
gcc -O3 -Wall -std=c11 message_sender.c -o message_sender
gcc -O3 -Wall -std=c11 message_reader.c -o message_reader

make

sudo dmesg --clear

sudo rmmod message_slot
sudo insmod message_slot.ko

sudo rm /dev/ofir_dev0
sudo mknod /dev/ofir_dev0 c 235 0
sudo chmod 666 /dev/ofir_dev0 

sudo rm /dev/ofir_dev1
sudo mknod /dev/ofir_dev1 c 235 1
sudo chmod 666 /dev/ofir_dev1