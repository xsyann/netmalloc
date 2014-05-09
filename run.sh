#!/usr/bin/env bash
##
## run.sh
##
## Made by xsyann
## Contact <contact@xsyann.com>
##
## Started on  Fri May  9 11:55:26 2014 xsyann
## Last update Fri May  9 11:57:14 2014 xsyann
##

module="netmalloc"

sudo dmesg -c > /dev/null

echo "   ┌───────────────────────────────────────────────────┐"
echo "-> │              # insmod netmalloc.ko                │"
echo "   └───────────────────────────────────────────────────┘"
make load
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
