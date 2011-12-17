#!/usr/bin/bash
make -C /usr/src/linux-2.6.37.4 SUBDIRS=/root/tamagotchi modules && insmod tamagotchi.ko
