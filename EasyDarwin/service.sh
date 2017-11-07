#!/bin/bash
#
#description: easydarwin service
#chkconfig:2345 88 77

lockfile=/var/lock/subsys/easydarwin
base_dir=/mnt/hgfs/ShareFolder/project/ED/EasyDarwin
target=./Debug/easydarwin
startup_script=startUpInDebug.sh
port=8888

# start
start(){
    if [ -e $lockfile ];then
        echo "Service is already running ..."
    elif [ $(ps -ef | grep "$target" | grep -v "grep" | wc -l) -eq 1 ];then
        echo "Program is already running but not with service ..."
    else
        touch $lockfile
        source /etc/profile
        cd $base_dir
        sh $startup_script
        echo "Service started ..."
        status       
    fi
}
#stop
stop(){
    if [ -e $lockfile ] ; then
	if  [ $(ps -ef | grep "$target" | grep -v "grep" | grep -v "/etc/init.d" | wc -l) -eq 1 ];then
            kill -9 $(ps -ef | grep "$target" | grep -v "grep"|grep -v "/etc/init.d"|awk '{print $2}')
                if [ $? -eq 0 ];then	
                    rm -f $lockfile
                    echo "Service stopped."
                else
                    echo "kill fail"
                fi
        else
            ps -ef | grep "$target" | grep -v "grep"
            echo $lockfile existing but process not running
        fi
    else
        echo "Service is not running."
    fi
 
}
#restart
restart(){
    stop
    start
}
usage(){
    echo "Usage:{start|stop|restart|status}"
}
status(){
    if [ -e $lockfile ];then
        echo "Service is running ..."
    elif [ $(ps -ef | grep "$target" | grep -v "grep" | wc -l) -eq 1 ];then
        echo "Program is already running but not with service ..."
    else
        echo "Service is not running."
    fi
    lsof -i:$port
}
case $1 in
start)
    start
    ;;
stop)
    stop
    ;;
restart)
    restart
    ;;
status)
    status
    ;;
*)
    usage
    exit 7
    ;;
esac

# cp this file to /etc/init.d/easydarwin to register as service
# and run "chkconfig --add easydarwin" for PowerBoot
# add task of restart service daily on 2:00 to crontab: 
# crontab -e
# then input:
# * 2 * * * service easydarwin restart
