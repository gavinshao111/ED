#!/bin/sh
# 
# File:   startUpInDebug.sh
# Author: 10256
#
# Created on 2017-1-19, 11:31:33
#
date > Logs/running.log
date_str=$(date -d "today" +"%Y%m%d")
./Debug/easydarwin -c WinNTSupport/easydarwin.xml -d >> Logs/running_${date_str}.log 2>>Logs/stderr_${date_str}.log &
sleep 2
tail -n 10 Logs/running_${date_str}.log
