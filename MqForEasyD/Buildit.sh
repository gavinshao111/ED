g++ strlfunc.cpp mainProcess.cpp -fpic -shared -I../../EasyDarwin-master/paho.mqtt.c/src -L../../EasyDarwin-master/paho.mqtt.c/build/output -lpaho-mqtt3a -lpaho-mqtt3as -lpaho-mqtt3c -lpaho-mqtt3cs -DNO_PERSISTENCE=1 -o libMqForEasyD.so -g