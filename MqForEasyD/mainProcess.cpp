
#include <string.h>
#include <unistd.h>
#include <cstdlib>
#include <iostream>
#include <sys/time.h>
using namespace std;
extern "C"{
    #include "MQTTAsync.h"
    #include "MQTTClient.h"
}
#include "strlfunc.h"
#include "mainProcess.h"

#define UINT unsigned int 

#define QOS         1
#define TIMEOUT     10000L

#define MQTTCLIENT_PERSISTENCE_NONE 1
#define URLERR if ('\0' == *(req+i+3)){fprintf(stderr, "URL Format error.\n");return 9;}
#define AURLERR if ('\0' == *(aVideoReqInfo->req+i+3)){fprintf(stderr, "URL Format error.\n");return 9;}
#define PRINTERR(ERRTYPE) fprintf(stderr, "%s format error:\n%s\n", (ERRTYPE), req);return 10;
//MQ
#define MAXPATHLEN 100
const char *strClientIdForMQ = "EasyDarwin";
//const char *strMQServerAddress = "tcp://120.27.188.84:1883";
const char *strMQServerAddress = "ssl://120.27.188.84:8883";


//RTSP
const char *strVideoinfoAsk = "videoinfoAsk";
//EasyDarwin与车机的MQ
const char *strServiceType = "viedoPlayer";
const char *strData_Type = "Realtime";
const char *strVideoType = "SD";
const char *strOperationBegin = "Begin";
const char *strOperationStop = "Stop";
//in code, only map first 9 chars.(LeapMotor)
const char *strCarUserAgent = "LeapMotor Push v1.0";

const UINT maxPayLoadLen= 2000;
const UINT maxTopicLen= 500;

void videoReqInfoType::print(void){
/**
    const char *req;
    int ipOfst;
    int portOfst;
    int realOrRecFlagOfst;
    int clientIdOfst;
    int videoTypeOfst;
    int fileNameOfst;
    int fileNameEndOfst;
    int userAgentOfst;
    bool ignore;
 */
    if ( NULL != req){
        fprintf(stderr, "req: %s\n\n"
                "ipOfst: %c, portOfst: %c, realOrRecFlagOfst: %c, clientIdOfst: %c, videoTypeOfst: %c, fileNameOfst: %c, fileNameEndOfst: %c, userAgentOfst: %c, ignore: %d\n\n", 
                req, *(req+ipOfst), *(req+portOfst), *(req+realOrRecFlagOfst), *(req+clientIdOfst), *(req+videoTypeOfst), *(req+fileNameOfst), *(req+fileNameEndOfst), *(req+userAgentOfst), (int)ignore);
    }
    else
        fprintf(stderr, "req: null\n\n", req);
    
}

int parseReq(const char *areq, videoReqInfoType* aVideoReqInfo, const bool startFromIp)
{
    if (NULL == areq || NULL == aVideoReqInfo)
        return -1;
    
    UINT i = 0;
    UINT j = 0;
    // step over space.
    for(;' ' == *(areq+i); i++)
        if('\0' == *(areq+i))
            return -1;
    aVideoReqInfo->req = areq+i;
    i = 0;
    if(!startFromIp) {
        if ('O' != *(aVideoReqInfo->req+i) && 'o' != *(aVideoReqInfo->req+i)){
            aVideoReqInfo->ignore = true;
            goto getUserAgentAndRet;
        }

        // step over "OPTION", point to next space.
        for(;' ' != *(aVideoReqInfo->req+i); i++)
            if('\0' == *(aVideoReqInfo->req+i))
                return -1;       
            
        if (*(aVideoReqInfo->req+ ++i) != 'r' ||
                *(aVideoReqInfo->req+ ++i) != 't' ||
                *(aVideoReqInfo->req+ ++i) != 's' ||
                *(aVideoReqInfo->req+ ++i) != 'p' ||
                *(aVideoReqInfo->req+ ++i) != ':' ||
                *(aVideoReqInfo->req+ ++i) != '/' ||
                *(aVideoReqInfo->req+ ++i) != '/')
            //PRINTERR("RTSP")
            return -2;
    
        ++i;
    }
    aVideoReqInfo->ipOfst = i;
    
    for (; ':' != *(aVideoReqInfo->req+i); i++){AURLERR}
    aVideoReqInfo->portOfst = ++i;
    
    for (;; i++) {
        if ('\0' == *(aVideoReqInfo->req+i)){
            return -4;
        }
        if (' ' == *(aVideoReqInfo->req+i)) {   // for case 4
            aVideoReqInfo->ignore = true;
            goto getUserAgentAndRet;           
        }
        else if ('/' == *(aVideoReqInfo->req+i)) {
            if (' ' == *(aVideoReqInfo->req+ ++i)){   // for case 3
                aVideoReqInfo->ignore = true;
                goto getUserAgentAndRet;
            }
            break;
        }                        
    }
    // for case 5 "192.168.43.201:8888/record/123/1/2016-08-30_113613.sdp" doesn't have next space.
    if (!startFromIp) {
        j = i;
        // step over "*.*", point to next space.
        for(; ' ' != *(aVideoReqInfo->req+j); j++){
            if ('\0' == *(aVideoReqInfo->req+j)){
                return -4;
            }    
        }

        if ('p' != *(aVideoReqInfo->req+ --j) ||
            'd' != *(aVideoReqInfo->req+ --j) ||
            's' != *(aVideoReqInfo->req+ --j) ||
            '.' != *(aVideoReqInfo->req+ --j)) {
            aVideoReqInfo->ignore = true;
            goto getUserAgentAndRet;    
        }
    }
    
    aVideoReqInfo->realOrRecFlagOfst = i;    
    
    for (; '/' != *(aVideoReqInfo->req+i); i++){AURLERR}
    if ('$' != *(aVideoReqInfo->req+ ++i)) {
        //PRINTERR("ClientId")
        return -12;
    }

    aVideoReqInfo->clientIdOfst = ++i;
    
    for (; '/' != *(aVideoReqInfo->req+i); i++){AURLERR}
    if ('0' != *(aVideoReqInfo->req+ ++i) && '1' != *(aVideoReqInfo->req+i)) {
        //PRINTERR("VideoType")
        return -13;
    }
    aVideoReqInfo->videoTypeOfst = i;
    
    if ('/' != *(aVideoReqInfo->req+ ++i)) {
        //PRINTERR("FileName")
        return -14;
    }

    aVideoReqInfo->fileNameOfst = ++i;

    for (; '\0' != *(aVideoReqInfo->req+i) && ' ' != *(aVideoReqInfo->req+i) && '/' != *(aVideoReqInfo->req+i); i++);
    aVideoReqInfo->fileNameEndOfst = i;


getUserAgentAndRet:
    for(;;i++){
        if(0 == *(aVideoReqInfo->req+i)){	//some req doesn't contain userAgent, userAgentOfst set to 0.
            aVideoReqInfo->ignore = true;
            aVideoReqInfo->userAgentOfst = 0;
            return 0;
        }
        else if (*(aVideoReqInfo->req+i) != 'U' ||
            *(aVideoReqInfo->req+ ++i) != 's' ||
            *(aVideoReqInfo->req+ ++i) != 'e' ||
            *(aVideoReqInfo->req+ ++i) != 'r' ||
            *(aVideoReqInfo->req+ ++i) != '-' ||
            *(aVideoReqInfo->req+ ++i) != 'A' ||
            *(aVideoReqInfo->req+ ++i) != 'g' ||
            *(aVideoReqInfo->req+ ++i) != 'e' ||
            *(aVideoReqInfo->req+ ++i) != 'n' ||
            *(aVideoReqInfo->req+ ++i) != 't' ||
            *(aVideoReqInfo->req+ ++i) != ':')
            continue;
        else    //get User-Agent:
            break;        
    }
    for (i++ ; ' ' == *(aVideoReqInfo->req+i); i++);
    aVideoReqInfo->userAgentOfst = i;
    if (false == aVideoReqInfo->ignore && 0 == memcmp(aVideoReqInfo->req+i, strCarUserAgent, 9) )
        aVideoReqInfo->ignore = true;
    
    return 0;
}


static int generateTopicAndPayLoad(const videoReqInfoType* aVideoReqInfo, char* strTopic, char *strPayLoad, const bool& isBegin)
{
    if (NULL == aVideoReqInfo || NULL == strTopic || NULL == strPayLoad)
        return -1;	

    *strTopic = '/';
    memcpy(strTopic + 1, aVideoReqInfo->req+aVideoReqInfo->clientIdOfst, aVideoReqInfo->videoTypeOfst - aVideoReqInfo->clientIdOfst);
    strlcpy(strTopic + 1 + aVideoReqInfo->videoTypeOfst - aVideoReqInfo->clientIdOfst, strVideoinfoAsk, maxPayLoadLen);    
        
    strlcat(strPayLoad, "{\"ServiceType\":\"", maxPayLoadLen);
    strlcat(strPayLoad, strServiceType, maxPayLoadLen);
    strlcat(strPayLoad, "\",\"Data_Type\":\"", maxPayLoadLen);
    
    if (0 == memcmp(aVideoReqInfo->req+aVideoReqInfo->realOrRecFlagOfst, "realtime", 8))
        strlcat(strPayLoad, "Realtime", maxPayLoadLen);
    else
        strlcat(strPayLoad, "Recording", maxPayLoadLen);
    
    strlcat(strPayLoad, "\",\"URL\":\"rtsp://", maxPayLoadLen);
    strncat(strPayLoad, aVideoReqInfo->req + aVideoReqInfo->ipOfst, aVideoReqInfo->fileNameEndOfst - aVideoReqInfo->ipOfst);
    strlcat(strPayLoad, "\",\"VideoType\":\"", maxPayLoadLen);
    
    if(isBegin) {   //this is unnecessary filed when Stop
        if ('0' == *(aVideoReqInfo->req + aVideoReqInfo->videoTypeOfst))
            strlcat(strPayLoad, "HD", maxPayLoadLen);
        else
            strlcat(strPayLoad, "SD", maxPayLoadLen);
    }
    
    strlcat(strPayLoad, "\",\"Operation\":\"", maxPayLoadLen);
    if (isBegin)
        strlcat(strPayLoad, strOperationBegin, maxPayLoadLen);
    else
        strlcat(strPayLoad, strOperationStop, maxPayLoadLen);

    strlcat(strPayLoad, "\",\"Datetime\":\"", maxPayLoadLen);
    struct timeval s_time;
    gettimeofday(&s_time, NULL);
    char strTime[20] ={0};
    sprintf(strTime, "%ld", ((long)s_time.tv_sec)*1000+(long)s_time.tv_usec/1000);
    strlcat(strPayLoad, strTime, maxPayLoadLen);

    strlcat(strPayLoad, "\"}", maxPayLoadLen);
	
    return 0;
}

int sendBeginOrStopPushMq(const videoReqInfoType* aVideoReqInfo, const int& timeout, const bool& isBegin) {
 
    if (NULL == aVideoReqInfo)
        return -1;        
    
    //char *strTopic = (char*)malloc(1 + videoTypeOfst - clientIdOfst + strlen(strVideoinfoAsk) + 2);
    char strTopic[maxTopicLen] = {0};
    char strPayLoad[maxPayLoadLen] = {0};
    if (0 != generateTopicAndPayLoad(aVideoReqInfo, strTopic, strPayLoad, isBegin))
        return -2;
    
    int rc = publishMq(strMQServerAddress, strClientIdForMQ, strTopic, strPayLoad, timeout);
    if (0 != rc){
        fprintf(stderr, "publishMq fail, return code: %d\n", rc);
        return -3;
    }
    
    return 0;    
}


int sendStopPushMq(const char *urlWithoutRTSP, const int& timeout) {

    if (NULL == urlWithoutRTSP)
        return -1;
    
    videoReqInfoType videoReqInfo={0};
    
    int rc = parseReq(urlWithoutRTSP, &videoReqInfo, true);
    if (0 != rc) {
        fprintf(stderr, "parseReq fail, return code: %d\n", rc);
        return -2; 
    }

    rc = sendBeginOrStopPushMq(&videoReqInfo, timeout, false);
    if (0 != rc) {
        fprintf(stderr, "sendBeginOrStopPushMq fail, return code: %d\n", rc);
        return -3;
    }
    
    return 0; 
}


int publishMq(const char *url, const char *clientId, const char *Topic, const char *PayLoad, const int& timeout) {
    MQTTClient client;
    int rc = 0;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    conn_opts.connectTimeout = timeout;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;

    if (rc = MQTTClient_create(&client, url, clientId,
            MQTTCLIENT_PERSISTENCE_NONE, NULL) != MQTTCLIENT_SUCCESS)
    {
        fprintf(stderr, "Failed to connect create MQTTClient, return code %d\n", rc);
        return -1;
    }
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.username = "easydarwin";
    conn_opts.password = "123456";

#if 1
    char pathOfServerPublicKey[MAXPATHLEN] = {0};
    char pathOfPrivateKey[MAXPATHLEN] = {0};
    snprintf(pathOfPrivateKey, MAXPATHLEN - 1, "%s/MqForEasyD/emqtt.key", getenv("ED"));
    snprintf(pathOfServerPublicKey, MAXPATHLEN - 1, "%s/MqForEasyD/emqtt.pem", getenv("ED"));
    MQTTClient_SSLOptions ssl_opts = MQTTClient_SSLOptions_initializer;
       
    ssl_opts.trustStore = pathOfServerPublicKey;
    ssl_opts.keyStore = pathOfServerPublicKey;
    ssl_opts.privateKey = pathOfPrivateKey;   
    ssl_opts.enableServerCertAuth = 0;
    conn_opts.ssl = &ssl_opts;
#endif

    
    if (rc = MQTTClient_connect(client, &conn_opts) != MQTTCLIENT_SUCCESS)
    {
        fprintf(stderr, "Failed to connect to MQ server, return code %d\n", rc);
        return -2;
    }
    pubmsg.payload = (void *)PayLoad;
    pubmsg.payloadlen = strlen(PayLoad);
    pubmsg.qos = QOS;
    pubmsg.retained = 0;
    if (rc = MQTTClient_publishMessage(client, Topic, &pubmsg, &token) != MQTTCLIENT_SUCCESS)
    {
        fprintf(stderr, "Failed to publishMessage to MQ server, return code %d\n", rc);
        return -3;
    }              

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    
    return 0;
}
