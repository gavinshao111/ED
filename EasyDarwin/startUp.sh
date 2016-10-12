#echo -e "\n\n" > Logs/running.log
date > Logs/running.log
./x64/easydarwin -c WinNTSupport/easydarwin.xml -d >> Logs/running.log 2>Logs/stderr.log &


