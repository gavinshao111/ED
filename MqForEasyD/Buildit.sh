g++ strlfunc.cpp mainProcess.cpp -fpic -shared -I$ED/paho.mqtt.c/src -L$ED/paho.mqtt.c/build/output -lpaho-mqtt3a -lpaho-mqtt3as -lpaho-mqtt3c -lpaho-mqtt3cs -DNO_PERSISTENCE=1 -o libMqForEasyD.so -g
g++ TestFile.cpp -L. -lMqForEasyD -o TestFile -g
./TestFile
