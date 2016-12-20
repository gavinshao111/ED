git add EasyDarwin/nbproject/Makefile-x64.mk
#add public OSRefTable::IsKeyExistingInTable
git add CommonUtilitiesLib/OSRef.cpp
git add CommonUtilitiesLib/OSRef.h

#  * in FindOrCreateSession, when push arrived add output. in RemoveOutputor, when push stop, add output then call cleanCV. implementation of IsUrlExistingInSessionMap
git add EasyDarwin/APIModules/QTSSReflectorModule/QTSSReflectorModule.cpp

# in ReflectorSession::RemoveOutput, dected that there is no APP in session, generate url and call StopPushMQ
# use default ip and port. need modify.
git add EasyDarwin/APIModules/QTSSReflectorModule/ReflectorSession.cpp

# when ED send DESC:404, it will call RTSPRequestInterface::WriteStandardHeaders, generate url and call StopPushMQgit and cleanCV
git add EasyDarwin/Server.tproj/RTSPRequestInterface.cpp

git add EasyDarwin/Server.tproj/RTSPRequestStream.cpp

# when send video data, rm log output in stdout.
git add EasyDarwin/Server.tproj/RTSPResponseStream.cpp

# git add ConditionVariable/cCondVB.h
# git add ConditionVariable/cCondVB.cpp
# git add ConditionVariable/Buildit.sh

git add EasyDarwin/WinNTSupport/easydarwin.xml
git add EasyDarwin/startUp*

# git add MqForEasyD/GetSession.h

git add MqForEasyD/mainProcess.cpp
git add MqForEasyD/mainProcess.h
# git add MqForEasyD/strlfunc.cpp
# git add MqForEasyD/strlfunc.h
 git add MqForEasyD/Buildit.sh
# git add MqForEasyD/emqtt.key
# git add MqForEasyD/emqtt.pem

# git add MqForEasyD/testFile.cpp

# git add MqForEasyD/MQTest/*.cpp
git add paho.mqtt.c/build/output/*
git add paho.mqtt.c/src/*.h


git add gitAddFiles.sh
git add json.c
git add CallStack.cpp
git add profile


# branch tmpSave
git add EasyDarwin/Server.tproj/QTSServer.cpp
git add EasyDarwin/Server.tproj/QTSServerInterface.*
git add EasyDarwin/RTSPReqInfo/RTSPReqInfo.h
git add EasyDarwin/RTSPReqInfo/RTSPReqInfo.cpp
git add CommonUtilitiesLib/StrPtrLen.cpp
git add CommonUtilitiesLib/StrPtrLen.h

git add emqtt.key
git add emqtt.pem