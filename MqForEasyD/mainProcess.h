#ifndef MAINPROCESS_H
#define MAINPROCESS_H

typedef struct sVideoReqInfoType{
    const char *req;
    int urlOfst;
    int ipOfst;
    int portOfst;
    int realOrRecFlagOfst;
    int clientIdOfst;
    int videoTypeOfst;
    int fileNameOfst;
    int fileNameEndOfst;
    int userAgentOfst;
    /* it is not app play req. like 
     *  1. OPTION rtsp://120.27.188.84:8888 RTSP/1.0\r\n CSeq: 17\r\n
     *  2. *.mp4
     *  3. PLAY or SETUP or other non-OPTION req.
     *  4. UserAgent is LeapMotor Push v1.0
     * only deal app option sdp req, like OPTION rtsp://10.34.16.180:8888/realtime/$1234/1/realtime.sdp
     */
    bool ignore;
}videoReqInfoType;

int sendBeginOrStopPushMq(const videoReqInfoType* aVideoReqInfo, const int& timeout, const bool& isBegin);

/*
 *  urlWithoutRTSP should like: 192.168.43.201:8888/realtime/$1234/1/realtime.sdp
 */
int sendStopPushMq(const char *urlWithoutRTSP, const int& timeout);

int publishMq(const char *url, const char *clientId, const char *Topic, const char *PayLoad, const int& timeout);

/*
 * areq maybe:
 *  1. OPTIONS rtsp://192.168.43.201:8888/*.mp4 RTSP/1.0\r\n...
 *  2. OPTIONS rtsp://192.168.43.201:8888/record/$1234/1/abc.sdp RTSP/1.0\r\n...
 *  3. OPTIONS rtsp://192.168.43.201:8888/ RTSP/1.0\r\n...
 *  4. OPTIONS rtsp://192.168.43.201:8888 RTSP/1.0\r\n...
 *  5. 192.168.43.201:8888/record/$carleapmotorCLOUDE20160727inform/1/2016-08-30_113613.sdp
 * 
 * return:
 * 0: ok.
 * negative: error.
 * 
 */
int parseReq(const char *areq, videoReqInfoType* aVideoReqInfo, const bool startFromIp);

#endif /* MAINPROCESS_H */

