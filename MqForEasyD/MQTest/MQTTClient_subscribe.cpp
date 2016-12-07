/*
g++ MQTTClient_subscribe.cpp -I$ED/paho.mqtt.c/src -L$ED/paho.mqtt.c/build/output -lpaho-mqtt3c -DNO_PERSISTENCE=1 -o sub
 */

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "MQTTClient.h"
#include <iostream>

#include<unistd.h>
#include<fcntl.h>  
#include<sys/types.h>  
#include<sys/stat.h>

using namespace std;

#define CLIENTID    "ExampleClientSub"
//#define TOPIC       "MQTT Examples"
#define PAYLOAD     "Hello World!"
#define QOS         1
#define TIMEOUT     10000L
#define MQTTCLIENT_PERSISTENCE_NONE 1
volatile MQTTClient_deliveryToken deliveredtoken;

int rwFd = -1;
const string ffmpegExecutableFilePath = "/mnt/hgfs/ShareFolder/ffmTest/out/test";
bool ffmpegIsPushing = false;
//bool ffmpegStatus;

int parseMQ(const string& MQPL, bool& operationIsBegin, string& url);
void sendCmdToFfmpeg(const bool& operationIsBegin, const string& url);
void CreateConnection();
void delivered(void *context, MQTTClient_deliveryToken dt);
int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message);
void connlost(void *context, char *cause);

int main(int argc, char* argv[]) {
    CreateConnection();
    string MQServerUrl = "tcp://120.27.188.84:1883";
    string topic = "/1234/videoinfoAsk";
    string tmp;
    char yOrN = 'n';

//    cout << "Default MQ Server URL is " << MQServerUrl << "  Ok? (y / n) ";
//    //cin>>tmp;
//    //if('n' == tmp.at(0) || 'N' == tmp.at(0)){
//    yOrN = getchar();
//    getchar();
//    if ('n' == yOrN || 'N' == yOrN) {
//        cout << "Input MQ Server URL: ";
//        cin>>MQServerUrl;
//    }
//
//
//    cout << "Default TOPIC is " << topic << "  Ok? (y / n) ";
//    yOrN = getchar();
//    getchar();
//    if ('n' == yOrN || 'N' == yOrN) {
//        cout << "Input TOPIC: ";
//        cin>>topic;
//    }

    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;
    int ch;

    MQTTClient_create(&client, MQServerUrl.c_str(), CLIENTID,
            MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    if (0 != MQServerUrl.compare(0, 15, "tcp://localhost")) {
        cout << "Connect with user name." << endl;
        conn_opts.username = "easydarwin";
        conn_opts.password = "123456";
    }

    MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }
    printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
            "Press Q<Enter> to quit\n\n", topic.c_str(), CLIENTID, QOS);
    MQTTClient_subscribe(client, topic.c_str(), QOS);

    do {
        ch = getchar();
    } while (ch != 'Q' && ch != 'q');

    write(rwFd, "s", 1);
//    sleep(1);
    close(rwFd);
    //close(socketFd);

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);

    return 0;
}

void delivered(void *context, MQTTClient_deliveryToken dt) {
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    int i;
    char* payloadptr;

    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: ");

    payloadptr = (char *) message->payload;
    for (i = 0; i < message->payloadlen; i++) {
        putchar(*payloadptr++);
    }
    putchar('\n');

    string MQPL((char *) message->payload, message->payloadlen);
    string url = "";
    bool operationIsBegin = false;

    int rc = parseMQ(MQPL, operationIsBegin, url);
    if (rc < 0) {
        cout << "parseMQ fail, rc = " << rc << endl;
    } else
        sendCmdToFfmpeg(operationIsBegin, url);

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

void connlost(void *context, char *cause) {
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

void CreateConnection(void) 
{
    char *fifoName = "/mnt/hgfs/ShareFolder/ffmTest/out/ffmTest";
    int res = -1;
    
    if (access(fifoName, F_OK) == -1) {
        res = mkfifo(fifoName, 0777);
        if (res != 0) {
            fprintf(stderr, "Could not create fifo %s\n", fifoName);
            exit(EXIT_FAILURE);
        }
    }
    
    rwFd = open(fifoName, O_WRONLY);
    if (rwFd < 0){
        cout << "open fifo " << fifoName << " fail." << endl;
        exit(EXIT_FAILURE);
    }
}

void sendCmdToFfmpeg(const bool& operationIsBegin, const string& url) {
    if (operationIsBegin) {
        if (ffmpegIsPushing)
            return;
        
        pid_t pid = vfork();
        if(pid == 0)
        {            
            cout << "child before execl" << endl;
            if (execl(ffmpegExecutableFilePath.data(), url.data(), NULL) < 0){
                perror("execl");
                exit(EXIT_FAILURE);
            }
        }
        
        cout << "ffmpeg pid " << pid << "started." << endl;
        sleep(1);
//        unsigned char urlLength = (unsigned char) url.length();
//        string buf = "b";
//        buf += urlLength;
//        buf += url;
//
//        int tmp = -1;
//        int writtenLength = 0;
//
//        for (; writtenLength < buf.length();) {
//            tmp = write(rwFd, buf.data() + writtenLength, buf.length() - writtenLength);
//            if (-1 == tmp) {
//                cout << "socket write fail." << endl;
//                exit(EXIT_FAILURE);
//            }
//            writtenLength += tmp;
//            tmp = -1;
//        }
//        cout << "send " << buf << " to ffmpeg." << endl;
        ffmpegIsPushing = true;
    } else {
        if (!ffmpegIsPushing)
            return;

        if (-1 == write(rwFd, "s", 1)) {
            cout << "fifo write fail. rwFd: " << rwFd << endl;
            exit(EXIT_FAILURE);
        }
        cout << "send s to ffmpeg." << endl;
        ffmpegIsPushing = false;
        //read(rwFd, readBuf, 1);
        // expect read 'y'        
    }
}

int parseMQ(const string& MQPL, bool& operationIsBegin, string& url) {
    if (MQPL.empty())
        return -1;
    size_t posOfUrl = string::npos;
    size_t posOfOperation = string::npos;

    const string UrlMarker = "\"URL\":\"";
    const string OperationMarker = "\"Operation\":\"";

    posOfUrl = MQPL.find(UrlMarker);
    if (string::npos == posOfUrl) {
        cout << "can't find " << UrlMarker << " in " << MQPL << endl;
        return -2;
    }

    posOfUrl += UrlMarker.length();

    url = MQPL.substr(posOfUrl, MQPL.find('\"', posOfUrl) - posOfUrl);

    posOfOperation = MQPL.find(OperationMarker, posOfUrl);
    if (string::npos == posOfOperation)
        return -3;
    posOfOperation += OperationMarker.length();

    if (0 == MQPL.compare(posOfOperation, 4, "Stop"))
        operationIsBegin = false;
    else if (0 == MQPL.compare(posOfOperation, 5, "Begin"))
        operationIsBegin = true;
    else
        return -4;

    return 0;
}
