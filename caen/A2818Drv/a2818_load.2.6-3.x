#!/bin/sh
module="a2818.ko"
device="a2818"
group="root"
mode="777"
script_path=`dirname $0`

 2> /dev/null
/sbin/rmmod $device 2> /dev/null
/sbin/insmod $script_path/$module || exit 1

rm -f /dev/${device}

major=`cat /proc/devices | awk "\\$2==\"$device\" {print \\$1}"`

rm -fr /dev/${device}_*

mknod /dev/${device}_0 c $major 0
mknod /dev/${device}_1 c $major 1
mknod /dev/${device}_2 c $major 2
mknod /dev/${device}_3 c $major 3
mknod /dev/${device}_4 c $major 4
mknod /dev/${device}_5 c $major 5
mknod /dev/${device}_6 c $major 6
mknod /dev/${device}_7 c $major 7
mknod /dev/${device}_16 c $major 16
mknod /dev/${device}_17 c $major 17
mknod /dev/${device}_18 c $major 18
mknod /dev/${device}_19 c $major 19
mknod /dev/${device}_20 c $major 20
mknod /dev/${device}_21 c $major 21
mknod /dev/${device}_22 c $major 22
mknod /dev/${device}_23 c $major 23
mknod /dev/${device}_32 c $major 32
mknod /dev/${device}_33 c $major 33
mknod /dev/${device}_34 c $major 34
mknod /dev/${device}_35 c $major 35
mknod /dev/${device}_36 c $major 36
mknod /dev/${device}_37 c $major 37
mknod /dev/${device}_38 c $major 38
mknod /dev/${device}_39 c $major 39

chown root   /dev/${device}_*
chgrp $group /dev/${device}_*
chmod $mode  /dev/${device}_*
