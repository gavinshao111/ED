#echo -e "\n\n" > Logs/running.log
date > Logs/running.log
./x64/easydarwin -c WinNTSupport/easydarwin.xml -d 1>Logs/running.log
