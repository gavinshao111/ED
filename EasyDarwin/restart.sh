# this script will restart easydarwin every 2:00

target=./Debug/easydarwin
startup_script=startUpInDebug.sh

hoursTo2clcok=$[26 - $(date +%H)]
sleep ${hoursTo2clcok}h
while :
do
    if [ $(ps -ef | grep "$target" | grep -v "grep" | wc -l) -eq 1 ]; then
        kill $(ps -ef | grep "$target" | grep -v "grep" | awk '{print $2}')
    fi
    sh $startup_script
    sleep 24h
done