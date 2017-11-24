#ifndef RTSPREQINFO_H
#define RTSPREQINFO_H


#include "StrPtrLen.h"
#include "OSRef.h"
#include "OSMutex.h"
#include "OSCond.h"
#include <string>
/*
 * RTSP req without fullFileName will be set to invaild, like OPTION rtsp://120.27.188.84:8888 RTSP/1.0\r\n CSeq: 17\r\n
 */
enum enumRTSPType {
    option, describe, announce, setup, play, record, teardown, invaild
};

void UnRegisterAndSendMQAndDelete(char *key, bool call_from_describe = false);
void on_rtsp_request(const StrPtrLen& req);
void notify(const char* vehicle);
void notify(const StrPtrLen& vehicle);
std::string now_str();
class PushInfo;

class RTSPReqInfo {
    /*
     * "rtsp://....sdp"
     * to {"ServiceType":"viedoPlayer","Data_Type":"Realtime","URL":"rtsp://120.27.188.84:8888/realtime/1234/1/realtime.sdp","VideoType":"SD","Operation":"Begin","Datetime":"1480735266671"}
     * or {"ServiceType":"viedoPlayer","Data_Type":"Realtime","URL":"rtsp://120.27.188.84:8888/realtime/1234/1/realtime.sdp","VideoType":"","Operation":"Stop","Datetime":"1480735281212"} 
     *
     * "rtsp://120.27.188.84:8888/record/phoneapptest/1/123.sdp"
     * 
     */

public:

    enumRTSPType RTSPType;

    StrPtrLen filePath;
    StrPtrLen fullUrl;
    StrPtrLen vehicleId;


    /*
     * LeapMotor Push v1.0 -> true
     * others -> false
     */
    bool isFromLeapMotor;

    RTSPReqInfo(const StrPtrLen& completeRequest);
    RTSPReqInfo(const RTSPReqInfo& orig);
    virtual ~RTSPReqInfo();

    void parseReqAndPrint(void);
    void Print(void);

private:

    PushInfo* pushInfo;
    StrPtrLen completeRequest;
    StrPtrLen userAgent;

};

/*
 * when app req arrived, registe into map and wait, then when another app with same url arrived,
 * reslove return PushInfo* duplicate, if(!duplicate->isPushArrived) call waitForPushArrived.
 * if timeout, do nothing.(RTSPRequestInterface.cpp return 404 then call UnRegister)
 * 
 * when push arrived, set pushInfo->isPushArrived = true, then RegisterOrResolve,
 * if return NULL, means there's no app wait for push, do nothing.
 * else return duplicate, set duplicate->isPushArrived = true, then call notify.
 *
 * when all apps leave, call UnRegister
 */
class PushInfo {
public:

    /*
     * it is key for every push
     * expect like record/phoneapptest/1/123.sdp
     */
    StrPtrLen filePath;

    /*
     * set true when wake up from wait.
     */
    bool isPushArrived;

    static OSMutex* fMutexForSendMQ;

    PushInfo();
    virtual ~PushInfo();

    bool parsePushInfo(const StrPtrLen& src);

    OSRef* GetRef() {
        return &fRef;
    }
    void readyToAddToTable(void);
    /*
     * send Mq if it is app's option or teardown
     */
    void sendBeginOrStopMq(bool isBegin);

    /*
     * if the push is not established, sendMQ then call this function to block until push arrived or timeout(8s).
     * rc:
     * true for receive push before timeout.
     * false for timeout
     */
    bool waitForNotification(const int& timeToWaitForPushArvd);

    void notifyApp(void);


private:

    // For storage in the session map
    OSRef fRef;
    /*
     * expect like "rtsp://ip:port/....sdp"
     * Allocate memeroy for it.
     */
    StrPtrLen fullUrl;

    /*
     * realtime -> true -> Realtime
     * record -> false -> Recording
     */
    bool isRealtime;

    StrPtrLen vehicleId;
    StrPtrLen MQPayLoad;

    /*
     * 0 -> true -> HD
     * 1 -> false -> SD
     */
    bool isHD;

    StrPtrLen startTime;
    StrPtrLen cameraIndex;
    StrPtrLen fileName;
    

    OSMutex fMutex;
    OSCond fCond;

    bool PublishMq(void) const;
    void toMQPayLoad(const bool& isBegin);


    friend class RTSPReqInfo;
    friend void on_rtsp_request(const StrPtrLen& req);
};


#endif /* RTSPREQINFO_H */
