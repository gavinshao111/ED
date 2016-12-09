#include "RTSPReqInfo.h"
#include "DateTranslator.h"
#include "QTSServerInterface.h"

const int timeToWaitForPush = 8;
const int timeOutForSendMQ = 4;
const char *strCarUserAgent = "LeapMotor Push v1.0";
char *strUserAgent = "User-Agent:";

void parseAndRegisterAndSendBeginMQAndWait(const StrPtrLen& req) {
    if (NULL == req.Ptr || req.Len < 1) {
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
            RTSPReqInfo* duplicateRTSPReqInfo = (RTSPReqInfo*) duplicateRef->GetObject();
            if (!duplicateRTSPReqInfo->isPushArrived) { // other app is wait for this key's push, I wait sa well.
                rtspReqInfo->sendBeginMQAndWaitForPushArrived(timeOutForSendMQ, timeToWaitForPush);
                usleep(1000 * 100); // at this time, motor and app are all in option, we delay app to let motor setup first.
                //rtspReqInfoTable->Release(duplicateRef);
            }
            // else other app is playing on this key.
            delete rtspReqInfo;
        }
    } else if (rtspReqInfo->isFromLeapMotor
            && rtspReqInfo->RTSPType == option) {
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
    RTSPReqInfo* rtspReqInfo = (RTSPReqInfo*) rtspReqInfoRef->GetObject();
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
    //filePath.Delete();
}

void RTSPReqInfo::readyToAddToTable(void) {
    fRef.Set(filePath, this);
}

void RTSPReqInfo::parseReqAndPrint(void) {
    completeRequest.TrimLeadingWhitespace();
    switch (completeRequest[0]) {
        case 'o':
        case 'O':
        {
            RTSPType = option;
            break;
        }
        case 'D':
        case'd':
        {
            RTSPType = describe;
            break;
        }
        case 'A':
        case 'a':
        {
            RTSPType = announce;
            break;
        }
        case 'S':
        case 's':
        {
            RTSPType = setup;
            break;
        }
        case 'P':
        case 'p':
        {
            RTSPType = play;
            break;
        }
        case 'R':
        case 'r':
        {
            RTSPType = record;
            break;
        }
        case 'T':
        case 't':
        {
            RTSPType = teardown;
            break;
        }
        default:
        {
            RTSPType = invaild;
            return;
        }
    }

    char* pos;
    // OPTIONS rtsp://192.168.43.201:8888/record/$1234/1/abc.sdp RTSP/1.0\r\n...
    do {
        if ((pos = completeRequest.FindNextChar(' ')) == NULL) break;
        urlStart.Ptr = ++pos;
        if ((pos = completeRequest.FindNextChar(' ', pos)) == NULL) break;
        urlStart.Len = pos - urlStart.Ptr;

        if (0 != strncmp(urlStart.Ptr, "rtsp://", 7)) break;

        // get / before record
        if ((pos = urlStart.FindNextChar('/', 7)) == NULL) break;
        filePath.Set(++pos, urlStart.Len + urlStart.Ptr - pos);

        if (0 == strncmp(filePath.Ptr, "record", 6))
            isRealtime = false;
        else if (0 == strncmp(filePath.Ptr, "realtime", 8))
            isRealtime = true;
        else break;

        // get / before 1234
        if ((pos = filePath.FindNextChar('/', pos)) == NULL) break;
        clientId.Ptr = ++pos;

        // get / before 1
        if ((pos = filePath.FindNextChar('/', pos)) == NULL) break;
        clientId.Len = pos - clientId.Ptr;

        // left at least should great then "1/_.sdp"
        if (pos + 7 > filePath.GetEnd()) break;

        if (*(++pos) == '0')
            isHD = true;
        else if (*pos == '1')
            isHD = false;
        else break;

        if (*(++pos) != '/') break;

        // expect end with .sdp
        if (0 != strncmp(filePath.GetEnd() - 3, ".sdp", 4)) break;

        if ((pos = completeRequest.FindString(strUserAgent)) == NULL) break;
        pos += strlen(strUserAgent);

        // skip space between "User-Agent:" and "LeapMotor Push v1.0"
        bool fail = false;
        for (; ' ' == *(pos++);)
            if (pos == completeRequest.GetEnd()) {
                fail = true;
                break;
            }
        if (fail) break;

        isFromLeapMotor = (0 == strcmp(pos, strCarUserAgent));

        DateBuffer theDate;
        DateTranslator::UpdateDateBuffer(&theDate, 0);
        fprintf(stderr, "************ %.6s %.9s %s TID: %lu\n\n",
                completeRequest.Ptr, pos, theDate.GetDateBuffer(), OSThread::GetCurrentThreadID());
        return;
    } while (false);
    RTSPType = invaild;
    return;
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
