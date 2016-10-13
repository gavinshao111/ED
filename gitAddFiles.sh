# no change, dont add them.
#git add EasyDarwin/APIModules/QTSSFileModule/QTSSFileModule.cpp
#git add EasyDarwin/Server.tproj/RTSPResponseStream.cpp
#git add EasyDarwin/Server.tproj/RTSPSession.cpp


git add nbproject/Makefile-x64.mk

#add public OSRefTable::IsKeyExistingInTable
git add CommonUtilitiesLib/OSRef.*

#  * in FindOrCreateSession, when push arrived add output. in RemoveOutputor, when push stop, add output then call cleanCV. implementation of IsUrlExistingInSessionMap
git add EasyDarwin/APIModules/QTSSReflectorModule/QTSSReflectorModule.cpp

# in ReflectorSession::RemoveOutput, dected that there is no APP in session, generate url and call StopPushMQ
git add EasyDarwin/APIModules/QTSSReflectorModule/ReflectorSession.cpp 

# when ED send DESC:404, it will call RTSPRequestInterface::WriteStandardHeaders, generate url and call StopPushMQgit and cleanCV
git add EasyDarwin/Server.tproj/RTSPRequestInterface.cpp

# pre-done
git add EasyDarwin/Server.tproj/RTSPRequestStream.cpp

# done
git add ConditionVariable/cCondVB.h
git add ConditionVariable/cCondVB.cpp
git add ConditionVariable/Buildit.sh
#done
git add EasyDarwin/WinNTSupport/easydarwin.xml
git add EasyDarwin/startUp*

# declare IsUrlExistingInSessionMap
git add MqForEasyD/GetSession.h
#done
git add MqForEasyD/mainProcess.cpp
git add MqForEasyD/mainProcess.h
git add MqForEasyD/strlfunc.cpp
git add MqForEasyD/strlfunc.h
git add MqForEasyD/Buildit.sh


git add MqForEasyD/stopReal.cpp
git add MqForEasyD/stopRec.cpp
git add MqForEasyD/MQTest/MQTTClient_publish.cpp
git add MqForEasyD/MQTest/MQTTClient_subscribe.cpp
git add MqForEasyD/MQTest/build.sh
git add MqForEasyD/beginReal.cpp
git add MqForEasyD/beginRec.cpp
git add MqForEasyD/buildForSend.sh

git add MqForEasyD/testFile.cpp
git add json.c
git add gitAddFiles.sh
git add log/ClientSetupInfo.log
git add log/log1.txt

git add paho.mqtt.c/build/output/libpaho-mqtt3a.so
git add paho.mqtt.c/build/output/libpaho-mqtt3a.so.1
git add paho.mqtt.c/build/output/libpaho-mqtt3a.so.1.0
git add paho.mqtt.c/build/output/libpaho-mqtt3as.so
git add paho.mqtt.c/build/output/libpaho-mqtt3as.so.1
git add paho.mqtt.c/build/output/libpaho-mqtt3as.so.1.0
git add paho.mqtt.c/build/output/libpaho-mqtt3c.so
git add paho.mqtt.c/build/output/libpaho-mqtt3c.so.1
git add paho.mqtt.c/build/output/libpaho-mqtt3c.so.1.0
git add paho.mqtt.c/build/output/libpaho-mqtt3cs.so
git add paho.mqtt.c/build/output/libpaho-mqtt3cs.so.1
git add paho.mqtt.c/build/output/libpaho-mqtt3cs.so.1.0
git add paho.mqtt.c/src/Clients.h
git add paho.mqtt.c/src/Heap.h
git add paho.mqtt.c/src/LinkedList.h
git add paho.mqtt.c/src/Log.h
git add paho.mqtt.c/src/MQTTAsync.h
git add paho.mqtt.c/src/MQTTClient.h
git add paho.mqtt.c/src/MQTTClientPersistence.h
git add paho.mqtt.c/src/MQTTPacket.h
git add paho.mqtt.c/src/MQTTPacketOut.h
git add paho.mqtt.c/src/MQTTPersistence.h
git add paho.mqtt.c/src/MQTTPersistenceDefault.h
git add paho.mqtt.c/src/MQTTProtocol.h
git add paho.mqtt.c/src/MQTTProtocolClient.h
git add paho.mqtt.c/src/MQTTProtocolOut.h
git add paho.mqtt.c/src/Messages.h
git add paho.mqtt.c/src/SSLSocket.h
git add paho.mqtt.c/src/Socket.h
git add paho.mqtt.c/src/SocketBuffer.h
git add paho.mqtt.c/src/StackTrace.h
git add paho.mqtt.c/src/Thread.h
git add paho.mqtt.c/src/Tree.h
git add paho.mqtt.c/src/utf-8.h