#!/usr/bin/env bash
##
## run.sh
##
## Made by xsyann
## Contact <contact@xsyann.com>
##
## Started on  Fri May  9 11:55:26 2014 xsyann
## Last update Sun May 25 16:24:42 2014 xsyann
##

module="netmalloc"

sudo dmesg -c > /dev/null

echo "   ┌───────────────────────────────────────────────────┐"
echo "-> │              # insmod netmalloc.ko                │"
echo "   └───────────────────────────────────────────────────┘"
make load ARGS=192.168.69.1:12345
if [ $? -ne 0 ]; then exit 1; fi

echo "   ┌───────────────────────────────────────────────────┐"
echo "-> │                  $ ./test/test                    │"
echo "   └───────────────────────────────────────────────────┘"
./test/test
echo "   ┌───────────────────────────────────────────────────┐"
echo "-> │               # rmmod netmalloc.ko                │"
echo "   └───────────────────────────────────────────────────┘"
sudo rmmod ${module}.ko
echo "   ┌───────────────────────────────────────────────────┐"
echo "-> │                      $ dmesg                      │"
echo "   └───────────────────────────────────────────────────┘"
dmesg
