#!/bin/sh
#Sync time & Insert every dd modules & Start program
echo Running shell cript program
rdate -s 203.248.240.140
date

insmod led_ioremap/ioremap_led_dd.ko
mknod /dev/iom_led c 246 0

insmod fnd_ioremap/ioremap_fnd_dd.ko
mknod /dev/iom_fnd c 241 0

insmod push_ioremap/ioremap_push_dd.ko
mknod /dev/iom_push c 244 0

insmod kernel_timer/clock_timer_dd.ko
mknod /dev/kernel_timer c 247 0

cd Clock
./Hangul_Clock
