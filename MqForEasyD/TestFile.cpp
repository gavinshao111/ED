/* 
g++ TestFile.cpp -L. -lMqForEasyD -o TestFile -g
*/ 
#include "mainProcess.h"
#include <iostream>
#include <assert.h>
#include <string.h>
#include <stdio.h>
using namespace std;

int main(void)
{
    
    const char *strCarUserAgent = "LeapMotor Push v1.0";
    const char *strOPTION = "OPTION";
    
    const char *req = " OPTIONS rtsp://10.34.16.143:8888/realtime/$1234/1/realtime.sdp RTSP/1.0 CSeq: 1 User-Agent: LeapMotor Push v1.0";
    
    videoReqInfoType videoReqInfo={0};
    assert(0 == parseReq(req, &videoReqInfo, false));
    assert(videoReqInfo.ignore);
    assert(0 != videoReqInfo.userAgentOfst);
    printf("%.9s\n", videoReqInfo.req + videoReqInfo.userAgentOfst);
    assert(0 == memcmp(videoReqInfo.req + videoReqInfo.userAgentOfst, strCarUserAgent, 9));
    
    //assert(0 == strcasecmp("Option", strOPTION));
    assert(0 == strncasecmp(videoReqInfo.req, strOPTION, strlen(strOPTION)));


    return 0;
}