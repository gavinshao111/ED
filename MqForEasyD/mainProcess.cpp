
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
#define AURLERR if ('\0' == *(areq+i+3)){fprintf(stderr, "URL Format error.\n");return 9;}
#define PRINTERR(ERRTYPE) fprintf(stderr, "%s format error:\n%s\n", (ERRTYPE), req);return 10;
//MQ
const char *strClientIdForMQ = "EasyDarwin";
const char *strMQServerAddress = "tcp://120.27.188.84:1883";
//const char *strMQServerAddress = "tcp://localhost:1883";
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


int parseReq(const char *areq, videoReqInfoType* aVideoReqInfo, const bool startFromIp)
{
    if (NULL == areq || NULL == aVideoReqInfo)
        return -1;
    
    UINT i = 0;
    UINT j = 0;
    // step over space., point to "O".
    for(;' ' == *(areq+i); i++)
        if('\0' == *(areq+i))
            return -1;
    aVideoReqInfo->req = areq+i;

    if(!startFromIp) {
        if ('O' != *(areq+i) && 'o' != *(areq+i)){
            aVideoReqInfo->ignore = true;
            goto getUserAgentAndRet;
        }

        // step over "OPTION", point to next space.
        for(;' ' != *(areq+i); i++)
            if('\0' == *(areq+i))
                return -1;       
    
        aVideoReqInfo->urlOfst = ++i;
        if (*(areq+i) != 'r' ||
                *(areq+ ++i) != 't' ||
                *(areq+ ++i) != 's' ||
                *(areq+ ++i) != 'p' ||
                *(areq+ ++i) != ':' ||
                *(areq+ ++i) != '/' ||
                *(areq+ ++i) != '/')
            //PRINTERR("RTSP")
            return -2;
    
        ++i;
    }
    aVideoReqInfo->ipOfst = i;
    
    for (; ':' != *(areq+i); i++){AURLERR}
    aVideoReqInfo->portOfst = ++i;
    
    for (;; i++) {
        if ('\0' == *(areq+i)){
            return -4;
        }
        if (' ' == *(areq+i)) {   // for case 4
            aVideoReqInfo->ignore = true;
            goto getUserAgentAndRet;           
        }
        else if ('/' == *(areq+i)) {
            if (' ' == *(areq+ ++i)){   // for case 3
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
        for(; ' ' != *(areq+j); j++){
            if ('\0' == *(areq+j)){
                return -4;
            }    
        }

        if ('p' != *(areq+ --j) ||
            'd' != *(areq+ --j) ||
            's' != *(areq+ --j) ||
            '.' != *(areq+ --j)) {
            aVideoReqInfo->ignore = true;
            goto getUserAgentAndRet;    
        }
    }
    
    aVideoReqInfo->realOrRecFlagOfst = i;    
    
    for (; '/' != *(areq+i); i++){AURLERR}
    if ('$' != *(areq+ ++i)) {
        //PRINTERR("ClientId")
        return -12;
    }

    aVideoReqInfo->clientIdOfst = ++i;
    
    for (; '/' != *(areq+i); i++){AURLERR}
    if ('0' != *(areq+ ++i) && '1' != *(areq+i)) {
        //PRINTERR("VideoType")
        return -13;
    }
    aVideoReqInfo->videoTypeOfst = i;
    
    if ('/' != *(areq+ ++i)) {
        //PRINTERR("FileName")
        return -14;
    }

    aVideoReqInfo->fileNameOfst = ++i;

    for (; '\0' != *(areq+i) && ' ' != *(areq+i) && '/' != *(areq+i); i++);
    aVideoReqInfo->fileNameEndOfst = i;


getUserAgentAndRet:
    for(;;i++){
        if(0 == *(areq+i)){	//some req doesn't contain userAgent, userAgentOfst set to 0.
            aVideoReqInfo->ignore = true;
            aVideoReqInfo->userAgentOfst = 0;
            return 0;
        }
        else if (*(areq+i) != 'U' ||
            *(areq+ ++i) != 's' ||
            *(areq+ ++i) != 'e' ||
            *(areq+ ++i) != 'r' ||
            *(areq+ ++i) != '-' ||
            *(areq+ ++i) != 'A' ||
            *(areq+ ++i) != 'g' ||
            *(areq+ ++i) != 'e' ||
            *(areq+ ++i) != 'n' ||
            *(areq+ ++i) != 't' ||
            *(areq+ ++i) != ':')
            continue;
        else    //get User-Agent:
            break;        
    }
    for (i++ ; ' ' == *(areq+i); i++);
    aVideoReqInfo->userAgentOfst = i;
    if (false == aVideoReqInfo->ignore && 0 == memcmp(areq+i, strCarUserAgent, 9) )
        aVideoReqInfo->ignore = true;
    
    return 0;
}

//int parseUrl(char *url, )

/*
 * 客户端发过来的URL例子:
rtsp://ip:port/realtime/$clientid/VideoType/realtime.sdp
VideoType=1 标清
VideoType=0 高清
 */
/*
 * req例子: 
OPTION rtsp://120.27.188.84:8888/realtime/$1234/0/realtime.sdp RTSP/1.0\r\n
CSeq: 17\r\n


DESCRIBE rtsp://172.17.4.9:8888/realtime/$carleapmotorCLOUDE20160727inform/1/realtime.sdp RTSP/1.0\r\n
CSeq: 3\r\n
User-Agent: LibVLC/2.2.4 (LIVE555 Streaming Media v2016.02.22)\r\n
Accept: application/sdp\r\n
\r\n
 *  */

static int generateTopicAndPayLoad(const videoReqInfoType* aVideoReqInfo, char* strTopic, char *strPayLoad, bool isBegin)
{
    if (NULL == aVideoReqInfo || NULL == strTopic || NULL == strPayLoad)
        return -1;
	
    //char strTopic[1 + videoTypeOfst - clientIdOfst + sizeof(strVideoinfoAsk)] = {0};
    //char* strTopic = (char *)malloc(sizeof(char)*(1 + videoTypeOfst - clientIdOfst + sizeof(strVideoinfoAsk)));
    //memset(strTopic, 0, sizeof(char)*(1 + videoTypeOfst - clientIdOfst + sizeof(strVideoinfoAsk)));
    //char *strTopic = (char*)malloc(1 + videoTypeOfst - clientIdOfst + strlen(strVideoinfoAsk) + 2);
    //memset(strTopic, 0, 1 + videoTypeOfst - clientIdOfst + strlen(strVideoinfoAsk) + 2);
         
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
    
    strlcat(strPayLoad, "\",\"URL\":\"", maxPayLoadLen);
    strncat(strPayLoad, aVideoReqInfo->req + aVideoReqInfo->urlOfst, aVideoReqInfo->fileNameEndOfst - aVideoReqInfo->urlOfst);
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

int sendStartPushMq(const videoReqInfoType* aVideoReqInfo) {
 
    if (NULL == aVideoReqInfo)
        return -1;        
    
    //char *strTopic = (char*)malloc(1 + videoTypeOfst - clientIdOfst + strlen(strVideoinfoAsk) + 2);
    char strTopic[maxTopicLen] = {0};
    char strPayLoad[maxPayLoadLen] = {0};
    if (0 != generateTopicAndPayLoad(aVideoReqInfo, strTopic, strPayLoad, true))
        return -2;
    
    int rc = publishMq(strMQServerAddress, strClientIdForMQ, strTopic, strPayLoad);
    if (0 != rc){
        fprintf(stderr, "publishMq fail, return code: %d\n", rc);
        return -3;
    }
    
    return 0;    
}

int sendStopPushMq(const char *urlWithoutRTSP) {

    if (NULL == urlWithoutRTSP)
        return -1;
    
    videoReqInfoType videoReqInfo={0};
    int rc = parseReq(urlWithoutRTSP, &videoReqInfo, true);
    if (0 != rc){
        fprintf(stderr, "parseReq fail, return code: %d\n", rc);
        return -2; 
    }
    char strTopic[maxTopicLen] = {0};
    char strPayLoad[maxPayLoadLen] = {0};
    rc = generateTopicAndPayLoad(&videoReqInfo, strTopic, strPayLoad, false);
    if (0 != rc){
        fprintf(stderr, "generateTopicAndPayLoad fail, return code: %d\n", rc);
        return -3;
    }
    rc = publishMq(strMQServerAddress, strClientIdForMQ, strTopic, strPayLoad);
    if (0 != rc){
        fprintf(stderr, "publishMq fail, return code: %d\n", rc);
        return -4;
    }
    
    return 0; 
}

/*
int sendStopPushMqWhenThereIsNoClient(const char *url){
#if IGNMQ
    return 0;
#endif  
    if (NULL == url)
        return -1;
    
    UINT clientIdOfst = -1;
    UINT endOfClientIdOfst = -1;
    UINT endOfFileNameOfst = -1;
    UINT realOrRecFlagOfst = -1;
    bool isRealtime = false;
    int i = 0;
    
    if (*(url+i) != 'r' ||
            *(url+ ++i) != 't' ||
            *(url+ ++i) != 's' ||
            *(url+ ++i) != 'p' ||
            *(url+ ++i) != ':' ||
            *(url+ ++i) != '/' ||
            *(url+ ++i) != '/')
        //PRINTERR("RTSP")
        return -11;
        
    //ipOfst = ++i;
    i++;
    
    for (; '/' != *(url+i); i++){
        if ('\0' == *(url+i)){
            fprintf(stderr, "URL Format error.\n");
            return -9;
        }    
    }
    realOrRecFlagOfst = ++i;
    
    if (0 == memcmp(url+i, "realtime", 8))
        isRealtime = true;
    else if (0 != memcmp(url+i, "record", 6))
        return -10;
    else
        isRealtime = false;
    
    for (; '$' != *(url+i); i++){
        if ('\0' == *(url+i))
            return -1;        
    }
    clientIdOfst = ++i;        
    
    for (; '/' != *(url+i); i++){
        if ('\0' == *(url+i))
            return -2;        
    }
    endOfClientIdOfst = i;
    if (endOfClientIdOfst <= clientIdOfst)
        return -3;
    for (; '/' != *(url+i); i++){
        if ('\0' == *(url+i))
            return -4;        
    }    
    for (;; i++){
        if ('\0' == *(url+i))
            return -5;
        else if ('.' == *(url + i) &&
                's' == *(url + ++i) &&
                'd' == *(url + ++i) &&
                'p' == *(url + ++i)) {
            endOfFileNameOfst = ++i;
            break;
        }
    }
    if (-1 == endOfFileNameOfst)
        return -6;
    
    //strTopic should like  "/carleapmotorCLOUDE20160727inform/videoinfoAsk";
    //char strTopic[endOfClientIdOfst - clientIdOfst + sizeof(strVideoinfoAsk) + 2] = {0};
    UINT lenOfStrTopic = endOfClientIdOfst - clientIdOfst + strlen(strVideoinfoAsk) + 4;
    char *strTopic = (char*)malloc(lenOfStrTopic);
    memset(strTopic, 0, lenOfStrTopic);
    *strTopic = '/';
    strncpy(strTopic + 1 , url + clientIdOfst, endOfClientIdOfst - clientIdOfst);
    strlcat(strTopic, "/", lenOfStrTopic);
    strlcat(strTopic, strVideoinfoAsk, lenOfStrTopic);

    char strPayLoad[maxPayLoadLen] = {0};    
    strlcat(strPayLoad, "{\"ServiceType\":\"", maxPayLoadLen);   
    strlcat(strPayLoad, strServiceType, maxPayLoadLen);
    strlcat(strPayLoad, "\",\"Data_Type\":\"", maxPayLoadLen);

    if (isRealtime)
        strlcat(strPayLoad, "Realtime", maxPayLoadLen);
    else
        strlcat(strPayLoad, "Recording", maxPayLoadLen);
    
    //strlcat(strPayLoad, strData_Type, maxPayLoadLen);
    strlcat(strPayLoad, "\",\"URL\":\"", maxPayLoadLen);
    //strlcat(strPayLoad, "rtsp://120.27.188.84:8888/", maxPayLoadLen);

    strncat(strPayLoad, url, endOfFileNameOfst);   
//    for(i = 0; 0 != *(fStreamName+i) &&  ' ' != *(fStreamName+i); i++)
//    {
//        *(strPayLoad+currPos+i) = *(fStreamName+i);
//    }    
    //strncat(strPayLoad, fStreamName, maxPayLoadLen);
    
    strlcat(strPayLoad, "\",\"VideoType\":\"", maxPayLoadLen);        
    strlcat(strPayLoad, "\",\"Operation\":\"", maxPayLoadLen);
    strlcat(strPayLoad, strOperationStop, maxPayLoadLen);

    strlcat(strPayLoad, "\",\"Datetime\":\"", maxPayLoadLen);
	struct timeval s_time;
	gettimeofday(&s_time, NULL);
	char strTime[20] ={0};
	sprintf(strTime, "%ld", ((long)s_time.tv_sec)*1000+(long)s_time.tv_usec/1000);
	strlcat(strPayLoad, strTime, maxPayLoadLen);
    
	strlcat(strPayLoad, "\"}", maxPayLoadLen);
	
//    char *strPayLoad = "{\"ServiceType\":\"\", \"Data_Type\": \"\", \"URL\":\"\", \"VideoType\":\"\" , \"Operation\":\"Stop\" }";    
    
    int rc = publishMq(strMQServerAddress, strClientIdForMQ, strTopic, strPayLoad);
    if (0 != rc){
        fprintf(stderr, "publishMq to StopPush fail, return code: %d\n", rc);
        free(strTopic);
        return -7;
    }
    
    free(strTopic);
    return 0;
}
*/

int publishMq(const char *url, const char *clientId, const char *Topic, const char *PayLoad) {
    MQTTClient client;
    int rc = 0;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    conn_opts.connectTimeout = 5;
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
