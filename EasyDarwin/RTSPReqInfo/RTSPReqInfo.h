#ifndef RTSPREQINFO_H
#define RTSPREQINFO_H


#include "StrPtrLen.h"
#include "OSRef.h"
#include "OSMutex.h"
#include "OSCond.h"

/*
 * RTSP req without fullFileName will be set to invaild, like OPTION rtsp://120.27.188.84:8888 RTSP/1.0\r\n CSeq: 17\r\n
 */
enum enumRTSPType {
    option, describe, announce, setup, play, record, teardown, invaild
};

void UnRegisterAndSendMQAndDelete(char *key);
void parseAndRegisterAndSendBeginMQAndWait(const StrPtrLen& req);

class RTSPReqInfo {
    /*
     * "rtsp://120.27.188.84:8888/realtime/1234/1/realtime.sdp"
     * to {"ServiceType":"viedoPlayer","Data_Type":"Realtime","URL":"rtsp://120.27.188.84:8888/realtime/1234/1/realtime.sdp","VideoType":"SD","Operation":"Begin","Datetime":"1480735266671"}
     * or {"ServiceType":"viedoPlayer","Data_Type":"Realtime","URL":"rtsp://120.27.188.84:8888/realtime/1234/1/realtime.sdp","VideoType":"","Operation":"Stop","Datetime":"1480735281212"} 
     *
     * "rtsp://120.27.188.84:8888/record/phoneapptest/1/123.sdp"
     * 
     */

public:

    enumRTSPType RTSPType;

    /*
     * LeapMotor Push v1.0 -> true
     * others -> false
     */
    bool isFromLeapMotor;
    
    /*
     * set true when wake up from wait.
     */
    bool isPushArrived;
    
    /*
     * it is key for every push
     * expect like record/phoneapptest/1/123.sdp
     */
    StrPtrLen filePath;

    RTSPReqInfo(const StrPtrLen& completeRequest);
    RTSPReqInfo(const RTSPReqInfo& orig);
    virtual ~RTSPReqInfo();

    
    OSRef* GetRef() { return &fRef; }
    void readyToAddToTable(void);
    
    /*
     * fprintf(stderr, "************ %.6s %.9s %s TID: %lu\n\n", fRequest.Ptr, fRequest.Ptr+videoReqInfo.userAgentOfst, theDate.GetDateBuffer(), OSThread::GetCurrentThreadID());
     */
    void parseReqAndPrint(void);

    /*
     * send Mq if it is app's option or teardown
     */
    void sendBeginMq(const int& timeout);
    void sendStopMq(const int& timeout);

    void waitForPush(const int& timeout);
    
    /*
     * if the push is not established, sendMQ then call this function to block until push arrived or timeout(8s).
     * rc:
     * true for receive push before timeout.
     * false for timeout
     */
    bool sendBeginMQAndWaitForPushArrived(const int& timeOutForSendMQ, const int& timeToWaitForPush);

    void notifyAppThatPushIsArrived(void);

    
    
private:
    // For storage in the session map
    OSRef fRef;
    
    OSMutex fMutex;
    OSCond  fCond;
    
    StrPtrLen completeRequest;
    /*
     * expect like "rtsp://ip:port"
     */
    StrPtrLen urlStart;

    /*
     * realtime -> true -> Realtime
     * record -> false -> Recording
     */
    bool isRealtime;

    StrPtrLen clientId;

    /*
     * 0 -> true -> HD
     * 1 -> false -> SD
     */
    bool isHD;

    StrPtrLen userAgent;



};

#endif /* RTSPREQINFO_H */

