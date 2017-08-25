# this script will restart easydarwin every 2:00

target=./Debug/easydarwin
startup_script=startUpInDebug.sh

restart(){
    if [ $(ps -ef | grep "$target" | grep -v "grep" | wc -l) -eq 1 ]; then

        # if ed is in close_wait, only kill -9 will work
        kill -9 $(ps -ef | grep "$target" | grep -v "grep" | awk '{print $2}')
        if [ $? -eq 0 ]; then
            echo killed
        else
            echo kill fail
            exit 1
        fi
    fi

    # 虽然上面kill已经返回0，但是此时进程没有退出，sleep 1s等进程退出
    sleep 1

    sh $startup_script
    echo restart done $(date)
}

hoursTo2clcok=$[26 - $(date +%H)]
#timeSleep=$[60 - $(date +%M)]

echo PID: $$
restart
sleep ${hoursTo2clcok}h
#sleep ${timeSleep}m

while :
do
    restart
    sleep 24h
done
