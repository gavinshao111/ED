#echo -e "\n\n" > Logs/running.log
date > Logs/running.log
./x64/easydarwin -c WinNTSupport/easydarwin.xml -d >> Logs/running_$(date -d "today" +"%Y%m%d_%H%M%S").log 2>Logs/stderr_$(date -d "today" +"%Y%m%d_%H%M%S").log &


