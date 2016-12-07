#include "RTSPReqInfo.h"


RTSPReqInfo::RTSPReqInfo(const StrPtrLen& completeRequest) {
    this->completeRequest = completeRequest;
    isPushArrived = false;
}

RTSPReqInfo::RTSPReqInfo(const RTSPReqInfo& orig) {
}

RTSPReqInfo::~RTSPReqInfo() {
}

void RTSPReqInfo::readyToAddToTable(void){
    fRef.Set(filePath, this);
}

void RTSPReqInfo::parseReqAndPrint(void) {
}

/*
 * send Mq if it is app's option or teardown
 */
void RTSPReqInfo::sendBeginMq(const int& timeout) {
}

void RTSPReqInfo::sendStopMq(const int& timeout) {
}

/*
 * if the push is not established, sendMQ then call this function to block until push arrived or timeout(8s).
 * rc:
 * true for receive push before timeout.
 * false for timeout
 */
bool RTSPReqInfo::sendBeginMQAndWaitForPushArrived(const int& timeOutForSendMQ, const int& timeToWaitForPush) {
    return true;
}

void RTSPReqInfo::notifyAppThatPushIsArrived(void) {
}
