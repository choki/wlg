#!/bin/bash
sudo hdparm -W 0 /dev/sda
sudo chmod 777 /sys/block/sda/queue/scheduler
echo noop > /sys/block/sda/queue/scheduler
cat /sys/block/sda/queue/scheduler


