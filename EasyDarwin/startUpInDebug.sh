#!/bin/sh
# 
# File:   startUpInDebug.sh
# Author: 10256
#
# Created on 2017-1-19, 11:31:33
#
date > Logs/running.log
./debug/easydarwin -c WinNTSupport/easydarwin.xml -d >> Logs/running_$(date -d "today" +"%Y%m%d_%H%M%S").log 2>Logs/stderr_$(date -d "today" +"%Y%m%d_%H%M%S").log &
