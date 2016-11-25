/*
g++ SendMQ.cpp -I.. -L.. -lMqForEasyD -o SendMQ -g
 */

#include "mainProcess.h"

#include <assert.h>
#include <iostream>
#include <string.h>
using namespace std;

int main(void) {

    int timeOutForSendMQ = 4;

    int j = 0;
    string cmd;
    string cmd2;
    string SReq;

    string url1 = "rtsp://120.26.86.124:8888/realtime/$leapmotorNo1/1/realtime.sdp";
    string url2 = "rtsp://120.26.86.124:8888/record/$leapmotorNo1/1/123.sdp";

    string url3 = "rtsp://120.27.188.84:8888/realtime/$leapmotorNo1/1/realtime.sdp";
    string url4 = "rtsp://120.27.188.84:8888/record/$leapmotorNo1/1/123.sdp";

    string url5 = "rtsp://120.27.188.84:8888/realtime/$1234/1/realtime.sdp";

    int UrlNum = 5;
    string UrlList[5] = {url1, url2, url3, url4, url5};



    cout << "choose a url:" << endl;
    for (; j < UrlNum; j++)
        cout << j + 1 << ". " << UrlList[j] << endl;

    cin>>cmd;

    cout << "Stop or Begin?\n1. Begin  2. Stop" << endl;
    cin>>cmd2;


    videoReqInfoType videoReqInfo = {0};
    
    SReq = "OPTIONS " + UrlList[cmd[0] - '1'] + " RTSP/1.0 CSeq: 1 User-Agent: Gavin Test v1.0";
    assert(0 == parseReq(SReq.data(), &videoReqInfo, false));


    if ('1' == cmd2[0])
        assert(0 == sendBeginOrStopPushMq(&videoReqInfo, timeOutForSendMQ, true));
    else if ('2' == cmd2[0])
        assert(0 == sendBeginOrStopPushMq(&videoReqInfo, timeOutForSendMQ, false));
    else
        cout << "input error" << endl;

    cout << "MQ Sent." << endl;

    return 0;
}