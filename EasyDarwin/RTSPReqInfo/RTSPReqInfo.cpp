#include "RTSPReqInfo.h"
#include "DateTranslator.h"
#include "QTSServerInterface.h"

const int timeToWaitForPush = 8;
const int timeOutForSendMQ = 4;

void parseAndRegisterAndSendBeginMQAndWait(const StrPtrLen& req) {
    if (NULL == req.Ptr || req.Len < 1){
        fprintf(stderr, "[ERROR] parseAndRegisterAndSendBeginMQAndWait: req invalid.\n\n");
        return;
    }
    DateBuffer theDate;
    OSRefTable* rtspReqInfoTable = QTSServerInterface::GetServer()->GetRTSPReqInfoMap();
    RTSPReqInfo* rtspReqInfo = new RTSPReqInfo(req);
    rtspReqInfo->parseReqAndPrint();
    fprintf(stderr, "[DEBUG] After parseReqAndPrint, rtspReqInfo->filePath: %.*s\n\n", rtspReqInfo->filePath.Len, rtspReqInfo->filePath.Ptr);
    if (!rtspReqInfo->isFromLeapMotor && rtspReqInfo->RTSPType == option) {
        rtspReqInfo->readyToAddToTable();
        OSRef* duplicateRef = rtspReqInfoTable->RegisterOrResolve(rtspReqInfo->GetRef());
        if (NULL == duplicateRef) { // Register successful
            fprintf(stderr, "[DEBUG] Register successful, key: %.*s\n\n", rtspReqInfo->filePath.Len, rtspReqInfo->filePath.Ptr);
            if (rtspReqInfo->sendBeginMQAndWaitForPushArrived(timeOutForSendMQ, timeToWaitForPush))
                usleep(1000 * 100); // at this time, motor and app are all in option, we delay app to let motor setup first.
            else {
                DateTranslator::UpdateDateBuffer(&theDate, 0);
                fprintf(stderr, "[INFO] %.*s: Wait for push timeout(%ds) %s TID: %lu\n\n", rtspReqInfo->filePath.Len, rtspReqInfo->filePath.Ptr, timeToWaitForPush, theDate.GetDateBuffer(), OSThread::GetCurrentThreadID());
                // rtspReqInfo will be delete in RTSPRequestInterface::WriteStandardHeaders(desc return 404).
            }
        } else {
            fprintf(stderr, "[DEBUG] Register fail, duplicate. key: %.*s\n\n", rtspReqInfo->filePath.Len, rtspReqInfo->filePath.Ptr);
            RTSPReqInfo* duplicateRTSPReqInfo = (RTSPReqInfo*)duplicateRef->GetObject();
            if (!duplicateRTSPReqInfo->isPushArrived) { // other app is wait for this key's push, I wait sa well.
                rtspReqInfo->sendBeginMQAndWaitForPushArrived(timeOutForSendMQ, timeToWaitForPush);
                usleep(1000 * 100); // at this time, motor and app are all in option, we delay app to let motor setup first.
                //rtspReqInfoTable->Release(duplicateRef);
            }
            // else other app is playing on this key.
            delete rtspReqInfo;
        }
    } else if (rtspReqInfo->isFromLeapMotor
            && rtspReqInfo->RTSPType == option){
        rtspReqInfo->notifyAppThatPushIsArrived();
        delete rtspReqInfo;
    }
}

void UnRegisterAndSendMQAndDelete(char *key) {
    OSRefTable* rtspReqInfoTable = QTSServerInterface::GetServer()->GetRTSPReqInfoMap();
    if (NULL == rtspReqInfoTable) {
        fprintf(stderr, "[ERROR] UnRegisterAndSendMQAndDelete: rtspReqInfoTable == NULL.\n\n");
        return;
    }
    StrPtrLen fullFileName(key);
    OSRef* rtspReqInfoRef = rtspReqInfoTable->Resolve(&fullFileName);
    if (NULL == rtspReqInfoRef) {
        fprintf(stderr, "[ERROR] UnRegisterAndSendMQAndDelete.Resolve fail, rtspReqInfoRef == NULL. key: %.*s\n\n", fullFileName.Len, fullFileName.Ptr);
        return;
    }
    fprintf(stderr, "[DEBUG] UnRegisterAndSendMQAndDelete.Resolve successful, key: %.*s\n\n", fullFileName.Len, fullFileName.Ptr);
    RTSPReqInfo* rtspReqInfo = (RTSPReqInfo*)rtspReqInfoRef->GetObject();
    rtspReqInfo->sendStopMq(timeOutForSendMQ);
    rtspReqInfoTable->UnRegister(rtspReqInfoRef, 999); //999 means froce remove regardless of refcount
    delete rtspReqInfo;
}

/*
 * we assert that RTSPRequestStream::fRequest is always existing before this obj deleted, so we needn't
 * new memory to save the request string.
 * otherwise, we need call this->completeRequest.AllocateAndCopy in construct and this->completeRequest.Delete in destruct
 */
RTSPReqInfo::RTSPReqInfo(const StrPtrLen& completeRequest) {
    this->completeRequest = completeRequest;
    isPushArrived = false;
}

RTSPReqInfo::RTSPReqInfo(const RTSPReqInfo& orig) {
}

RTSPReqInfo::~RTSPReqInfo() {
    // tmp
    filePath.Delete();
}

void RTSPReqInfo::readyToAddToTable(void) {
    fRef.Set(filePath, this);
}

void RTSPReqInfo::parseReqAndPrint(void) {
    filePath.AllocateAndCopy("realtime/$1234/1/realtime.sdp");
    fprintf(stderr, "[DEBUG] RTSPReqInfo::parseReqAndPrint filePath after AllocateAndCopy: %.*s\n\n", filePath.Len, filePath.Ptr);
    isFromLeapMotor = false;
    RTSPType = option;
}


void RTSPReqInfo::sendBeginMq(const int& timeout) {
    fprintf(stderr, "sendBeginMq() with timeout %d\n", timeout);
}

void RTSPReqInfo::sendStopMq(const int& timeout) {
    fprintf(stderr, "sendStopMq() with timeout %d\n", timeout);
}

/*
 * if the push is not established, sendMQ then call this function to block until push arrived or timeout(8s).
 * rc:
 * true for receive push before timeout.
 * false for timeout
 */
bool RTSPReqInfo::sendBeginMQAndWaitForPushArrived(const int& timeOutForSendMQ, const int& timeToWaitForPush) {
    fprintf(stderr, "sendBeginMQAndWaitForPushArrived with timeOutForSendMQ = %d timeToWaitForPush = %d\n", timeOutForSendMQ, timeToWaitForPush);
    return true;
}

void RTSPReqInfo::notifyAppThatPushIsArrived(void) {
    fprintf(stderr, "notifyAppThatPushIsArrived\n");
}
