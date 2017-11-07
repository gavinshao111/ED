/**
 * bug1：当app1在播放车机A的录像1时，app2请求播放车机A的录像2，8s超时然后发送stopMQ给车机A，车机就会停止，导致app1也播放结束
 * solution1: 车机端接收到stopMQ时，应判断 full url 是否与当前正在播放的url一致，一致则停止，否则忽略。
 * solution2: 将推流表的key从 filePath 换成 vehicleId ,app2 option请求到达时发现车机A已注册，继续走，
 * 然后desc404，这时再判断注册的推流信息中的filePath是否与当前请求filePath一致，若一致则可能车机推流太慢但待会会推，
 * 需要发送stopMQ，否则代表车机A在推其他视频，不发送MQ
 * 
 * bug2: 用vlc播放会出现问题：
 * 用我们的自己播放器播放时没有超时，当8s后没收到车机推流，处理option的线程醒来，然后rtsp流程继续，app发desc，ED返回desc404，返回调用
 * UnRegisterAndSendMQAndDelete() 发送stopMQ然后在注册表rtspReqInfoTable中删除，并删除pushInfo实例。
 * 而vlc自己有超时时间，大概是4s，若4s内没有收到视频数据，vlc不会再发任何RTSP命令给ED，所以pushInfo实例没被删除，注册表rtspReqInfoTable
 * 中也没被删除，下次请求过来时，注册表rtspReqInfoTable已存在，即认为当前推流BeginMQ已经发过，不再发送。
 * solution: 需要兼容这种情况，即客户端自带超时返回行为，
 * case1 车机没有推其他视频，app option timeout，然后后发送stopMQ,然后删除pushInfo实例
 * case2 车机正在推其他视频，基于bug1，当前app option 会立即返回404，不会有app自带超时返回的情况发生。
 * 
 * req1:
 * 录像拖动：filePath 视频起始播放时间变化，其他不变。比如从0s快进到50s，app option需要等到上一个，即0s
 * 的视频线路结束后再继续发送下一个请求，即50s。
 * solution1: 条件变量
 *  等待条件：rtspReqInfoTable中存在除了起始播放时间不同的filePath
 *  唤醒条件：可以是这个车机ID teardown的时候
 *  条件变量：车机ID
 * solution2: 在拖动的app option中强制sleep
 * 
 * 以上方案不可行，因为即使能保证拖完后的播放请求在上一次完成后（solution2已实现），之后的流程，
 * 车机option唤醒app option,但是motor setup 很大可能在app desc之后导致失败（不是同一个线程）
 * 明天让app端试试拖动后强制sleep，暂时不作修改
 */



#include "RTSPReqInfo.h"
#include "DateTranslator.h"
#include "QTSServerInterface.h"
#include <sys/time.h>

#include "mainProcess.h"

const char *strClientIdForMQ = "EasyDarwin";
const char *strMQServerAddress = "ssl://120.26.86.124:8883";

const int CTimeToWaitForPushArvd = 8;
const int timeOutForSendMQ = 4;
const char *strCarUserAgentValue = "LeapMotor Push";
char *strUserAgentKeyWord = "User-Agent:";
const int maxPayLoadLen = 2000;
const int maxTopicLen = 500;

OSMutex* PushInfo::fMutexForSendMQ = NULL;
namespace NMSRTSPReqInfo {
    size_t strlcat(char *dst, const char *src, size_t siz);
    size_t strlcpy(char *dst, const char *src, size_t siz);
    bool isDigital(StrPtrLen& src);
}

void parseAndRegisterAndSendBeginMQAndWait(const StrPtrLen& req) {
    if (NULL == req.Ptr || req.Len < 1) {
        fprintf(stderr, "[ERROR] parseAndRegisterAndSendBeginMQAndWait: req invalid.\n\n");
        return;
    }
    DateBuffer theDate;
    OSRefTable* rtspReqInfoTable = QTSServerInterface::GetServer()->GetRTSPReqInfoMap();
    if (NULL == rtspReqInfoTable) {
        fprintf(stderr, "[ERROR] UnRegisterAndSendMQAndDelete: rtspReqInfoTable == NULL.\n\n");
        return;
    }

    RTSPReqInfo rtspReqInfo(req);
    rtspReqInfo.parseReqAndPrint();
    if (rtspReqInfo.RTSPType == invaild) {

        return;
    }

    OSRef* pushInfoRef;
    PushInfo* pushInfo;

    /* the thread deal app option and deal motor setup is the same one, so block app option until motor setup is not work
     */
    bool MotorOption = rtspReqInfo.isFromLeapMotor && rtspReqInfo.RTSPType == option;
    bool MotorAnnounce = rtspReqInfo.isFromLeapMotor && rtspReqInfo.RTSPType == announce;
    bool AppOption = !rtspReqInfo.isFromLeapMotor && rtspReqInfo.RTSPType == option;
    bool MotorTeardown = rtspReqInfo.isFromLeapMotor && rtspReqInfo.RTSPType == teardown;

    if (MotorOption || AppOption || MotorTeardown) {

        pushInfoRef = rtspReqInfoTable->Resolve(&rtspReqInfo.vehicleId);
        DateTranslator::UpdateDateBuffer(&theDate, 0);
        if (NULL == pushInfoRef) { // 当前车机没有在推流，且在这之前没有其他app在等待当前车机播放
            if (MotorTeardown)
                return;
            if (MotorOption) {
                fprintf(stderr, "[WARN] %.*s: MotorOption arrived, but there's no app wait for push. %s TID: %lu\n\n",
                        rtspReqInfo.filePath.Len, rtspReqInfo.filePath.Ptr, theDate.GetDateBuffer(), OSThread::GetCurrentThreadID());
            }
            pushInfo = new PushInfo();
            if (!pushInfo->parsePushInfo(rtspReqInfo.fullUrl)) {
                DateTranslator::UpdateDateBuffer(&theDate, 0);
                fprintf(stderr, "[ERROR] %.*s: pushInfo->parsePushInfo fail. %s TID: %lu\n\n",
                        rtspReqInfo.filePath.Len, rtspReqInfo.filePath.Ptr, theDate.GetDateBuffer(), OSThread::GetCurrentThreadID());
                delete pushInfo;
                return;
            }

            pushInfo->readyToAddToTable();
            if (OS_NoErr != rtspReqInfoTable->Register(pushInfo->GetRef())) {
                DateTranslator::UpdateDateBuffer(&theDate, 0);
                fprintf(stderr, "[ERROR] %.*s: Register fail. %s TID: %lu\n\n",
                        rtspReqInfo.filePath.Len, rtspReqInfo.filePath.Ptr, theDate.GetDateBuffer(), OSThread::GetCurrentThreadID());
                delete pushInfo;
                return;
            }
            DateTranslator::UpdateDateBuffer(&theDate, 0);
            fprintf(stderr, "[INFO] %.*s: PushInfo allocated & registered. %s TID: %lu\n\n",
                    rtspReqInfo.filePath.Len, rtspReqInfo.filePath.Ptr, theDate.GetDateBuffer(), OSThread::GetCurrentThreadID());

        } else { // 推流信息已存在 may another app with same url is waiting, may push has arrived.
            pushInfo = (PushInfo*) pushInfoRef->GetObject();
            if (AppOption) {
                fprintf(stderr, "[DEBUG] %.*s: PushInfo is existing. %s TID: %lu\n\n",
                        rtspReqInfo.filePath.Len, rtspReqInfo.filePath.Ptr, theDate.GetDateBuffer(), OSThread::GetCurrentThreadID());
            } else if (MotorTeardown) { // 一般都会在发送stop mq以后删除pushInfo，所以到这里说明是车机主动断开

                rtspReqInfoTable->UnRegister(pushInfoRef, 0xffffffff);
                delete pushInfo;
                DateTranslator::UpdateDateBuffer(&theDate, 0);
                fprintf(stderr, "[INFO] %.*s: vehicle teardown by it self, pushInfo unregistered and deleted. %s TID: %lu\n\n",
                        rtspReqInfo.filePath.Len, rtspReqInfo.filePath.Ptr, theDate.GetDateBuffer(), OSThread::GetCurrentThreadID());
                return;
            }
        }

        DateTranslator::UpdateDateBuffer(&theDate, 0);
        if (MotorOption) {
            pushInfo->isPushArrived = true;
            fprintf(stderr, "[DEBUG] %.*s: pushInfo->isPushArrived set true, notifyAppThread wake up. %s TID: %lu\n\n",
                    pushInfo->filePath.Len, pushInfo->filePath.Ptr, theDate.GetDateBuffer(), OSThread::GetCurrentThreadID());
            pushInfo->notifyApp();
        } else if (!pushInfo->isPushArrived) { // app option && push hasn't arrived
            if (NULL == pushInfoRef) { // 未注册才发送BeginMQ
                pushInfo->sendBeginOrStopMq(true);
                fprintf(stderr, "[DEBUG] %.*s: AppOption & Push hasn't Arrived & hasn't registered, begin MQ sent. %s TID: %lu\n\n",
                        pushInfo->filePath.Len, pushInfo->filePath.Ptr, theDate.GetDateBuffer(), OSThread::GetCurrentThreadID());
            } else // 已注册但推流未到达时不再发送BeginMQ
                fprintf(stderr, "[DEBUG] %.*s: AppOption & Push hasn't Arrived & registered. %s TID: %lu\n\n", pushInfo->filePath.Len, pushInfo->filePath.Ptr, theDate.GetDateBuffer(), OSThread::GetCurrentThreadID());

            if (pushInfo->waitForNotification(CTimeToWaitForPushArvd)) {
                usleep(1000 * 500); // at this time, motor and app are all in option, we delay app to let motor setup first.
            } else {
                DateTranslator::UpdateDateBuffer(&theDate, 0);

                /*
                 * for bug2, 因为如果app自带超时提前返回，不会进入desc404的流程，所以需要在这里就释放资源；
                 * 若app不提前返回，在这里资源被释放，在进入desc404后，发现这个filePath未被注册，do nothing
                 */
                fprintf(stderr, "[INFO] %.*s: Wait for push timeout(%ds). %s TID: %lu\n\n",
                        rtspReqInfo.filePath.Len, rtspReqInfo.filePath.Ptr, CTimeToWaitForPushArvd, theDate.GetDateBuffer(), OSThread::GetCurrentThreadID());
                // in case that 2 apps wait for a same url push but timeout, first one call UnRegister the url and another call again, Resolve will fail.                    
                if (NULL == rtspReqInfoTable->ResolveAndUnRegister(&rtspReqInfo.vehicleId)) {
                    fprintf(stderr, "[INFO] %.*s: PushInfo already deleted, nothing to do. %s TID: %lu\n\n",
                            rtspReqInfo.filePath.Len, rtspReqInfo.filePath.Ptr, theDate.GetDateBuffer(), OSThread::GetCurrentThreadID());
                    return;
                }
                pushInfo->sendBeginOrStopMq(false);
                delete pushInfo;
                DateTranslator::UpdateDateBuffer(&theDate, 0);
                fprintf(stderr, "[INFO] %.*s: PushInfo unregistered and deleted, stop MQ sent. %s TID: %lu\n\n",
                        rtspReqInfo.filePath.Len, rtspReqInfo.filePath.Ptr, theDate.GetDateBuffer(), OSThread::GetCurrentThreadID());
            }
        } else { // AppOption && PushArrived
            fprintf(stderr, "[DEBUG] %.*s: AppOption & PushArrived, app option thread will continue. %s TID: %lu\n\n",
                    rtspReqInfo.filePath.Len, rtspReqInfo.filePath.Ptr, theDate.GetDateBuffer(), OSThread::GetCurrentThreadID());
        }
    }

}

/**
 * 
 * @param key expected to be like "realtime/$1234/1/realtim.sdp"
 * @param call_from_describe
 */
void UnRegisterAndSendMQAndDelete(char *key, bool call_from_describe/* = false*/) {
    DateBuffer theDate;

    OSRefTable* rtspReqInfoTable = QTSServerInterface::GetServer()->GetRTSPReqInfoMap();
    if (NULL == rtspReqInfoTable) {
        fprintf(stderr, "[ERROR] UnRegisterAndSendMQAndDelete: rtspReqInfoTable == NULL.\n\n");
        return;
    }

    StrPtrLen fullFileName(key);
    StrPtrLen vehicleId;
    vehicleId.Ptr = fullFileName.FindNextChar('/') + 1;
    vehicleId.Len = fullFileName.FindNextChar('/', vehicleId.Ptr) - vehicleId.Ptr;
    PushInfo* pushInfo;
    OSRef* pushInfoRef = rtspReqInfoTable->Resolve(&vehicleId);
    if (NULL == pushInfoRef) {
        if (call_from_describe) {
            DateTranslator::UpdateDateBuffer(&theDate, 0);
            fprintf(stderr, "[INFO] %s: pushInfo hasn't registered, may released already. %s TID: %lu\n\n", key, theDate.GetDateBuffer(), OSThread::GetCurrentThreadID());
            return;
        }
        // 若出现未知bug，rtspReqInfoTable中pushInfo被删除，但是车机仍在推，在QTSSReflectorModule或ReflectorSession中调用此函数，强行构造PushInfo,发送stopMQ
        
        pushInfo = new PushInfo();
        char* cPathWithPrefix = new char[30 + fullFileName.Len];
        sprintf(cPathWithPrefix, "rtsp://ip:port/%s", key);
        StrPtrLen src(cPathWithPrefix);
        if (!pushInfo->parsePushInfo(src)) {
            DateTranslator::UpdateDateBuffer(&theDate, 0);
            fprintf(stderr, "[ERROR] %s: pushInfo->parsePushInfo fail. %s TID: %lu\n\n",
                    cPathWithPrefix, theDate.GetDateBuffer(), OSThread::GetCurrentThreadID());
            delete pushInfo;
            delete cPathWithPrefix;
            return;
        }
        delete cPathWithPrefix;
        DateTranslator::UpdateDateBuffer(&theDate, 0);
        fprintf(stderr, "[WARN] %s: in some bug case, pushInfo not existed in rtspReqInfoTable but vehicle is pushing, forcibly new an instance to send stopMQ. %s TID: %lu\n\n", 
                key, theDate.GetDateBuffer(), OSThread::GetCurrentThreadID());
    } else
        pushInfo = (PushInfo*) pushInfoRef->GetObject();

    // for bug1
    if (!pushInfo->filePath.Equal(fullFileName)) {
        DateTranslator::UpdateDateBuffer(&theDate, 0);
        fprintf(stderr, "[DEBUG] %s: This vehicle is pushing another url: %.*s. Nothing to do. %s TID: %lu\n\n",
                key, pushInfo->filePath.Len, pushInfo->filePath.Ptr, theDate.GetDateBuffer(), OSThread::GetCurrentThreadID());
        return;
    }

    rtspReqInfoTable->UnRegister(pushInfoRef, 0xffffffff);
    pushInfo->sendBeginOrStopMq(false);
    delete pushInfo;
    DateTranslator::UpdateDateBuffer(&theDate, 0);
    fprintf(stderr, "[DEBUG] %s: PushInfo unregistered and deleted, stop MQ sent. %s TID: %lu\n\n",
            key, theDate.GetDateBuffer(), OSThread::GetCurrentThreadID());
}

/*
 * we assert that RTSPRequestStream::fRequest is always existing before this obj deleted, so we needn't
 * new memory to save the request string.
 * otherwise, we need call this->completeRequest.AllocateAndCopy in construct and this->completeRequest.Delete in destruct
 */
RTSPReqInfo::RTSPReqInfo(const StrPtrLen& completeRequest) {
    this->completeRequest = completeRequest;
}

RTSPReqInfo::RTSPReqInfo(const RTSPReqInfo& orig) {
}

RTSPReqInfo::~RTSPReqInfo() {
}

/*
 * OPTIONS rtsp://192.168.43.201:8888/record/1234/1/abc.sdp RTSP/1.0\r\n...
 * SETUP rtsp://10.34.16.127:8888/realtime/1234/1/realtime.sdp/trackID=0 RTSP/1.0\r\n
 * PLAY rtsp://10.34.16.127:8888/realtime/1234/1/realtime.sdp/
 * ANNOUNCE rtsp://10.34.16.127:8888/realtime/1234/1/realtime.sdp RTSP/1.0\r\n
 * 
 */
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
    char* posOfFilePathEnd;

    do {
        if (RTSPType == option || RTSPType == announce || RTSPType == teardown) {
            if ((pos = completeRequest.FindNextChar(' ')) == NULL) break;

            fullUrl.Ptr = ++pos;

            if (0 != strncmp(fullUrl.Ptr, "rtsp://", 7)) break;

            // get / before record
            if ((pos = completeRequest.FindNextChar('/', pos + 7)) == NULL) break;
            pos++;

            if ((posOfFilePathEnd = completeRequest.FindString(".sdp")) == NULL) break;
            posOfFilePathEnd += 4;

            filePath.Set(pos, posOfFilePathEnd - pos);
            fullUrl.Len = posOfFilePathEnd - fullUrl.Ptr;

            // get / before vehicle id
            if ((pos = completeRequest.FindNextChar('/', pos)) == NULL) break;
            vehicleId.Ptr = ++pos;
            // get / after vehicle id
            if ((pos = completeRequest.FindNextChar('/', pos)) == NULL) break;
            vehicleId.Len = pos - vehicleId.Ptr;

        }

        if ((pos = completeRequest.FindString(strUserAgentKeyWord)) == NULL) break;
        pos += strlen(strUserAgentKeyWord);

        // skip space between "User-Agent:" and "LeapMotor Push v1.0"
        bool fail = false;
        for (; ' ' == *pos; pos++)
            if (pos == completeRequest.GetEnd()) {
                fail = true;
                break;
            }
        if (fail) break;

        isFromLeapMotor = (0 == strncmp(pos, strCarUserAgentValue, strlen(strCarUserAgentValue)));

        DateBuffer theDate;
        DateTranslator::UpdateDateBuffer(&theDate, 0);
        fprintf(stderr, "************ %.6s %.9s %s TID: %lu\n\n",
                completeRequest.Ptr, pos, theDate.GetDateBuffer(), OSThread::GetCurrentThreadID());
        return;
    } while (false);
    RTSPType = invaild;
    return;
}

void PushInfo::readyToAddToTable(void) {
    //    fRef.Set(filePath, this);
    fRef.Set(vehicleId, this);
}

void PushInfo::sendBeginOrStopMq(bool isBegin) {
    toMQPayLoad(isBegin);
    OSMutexLocker locker(fMutexForSendMQ);

    if (!PublishMq() && !PublishMq()) // 最多重发两次
        fprintf(stderr, "[ERROR] publishMq fail.\n");
    else
        fprintf(stderr, "[DEBUG] MQ isBegin: %d sent.\n\n", isBegin);
}

/*
 * if the push is not established, sendMQ then call this function to block until push arrived or timeout(8s).
 * rc:
 * true for receive push before timeout.
 * false for timeout
 */
bool PushInfo::waitForNotification(const int& timeToWaitForPushArvd) {
    struct timeval s_time, e_time;
    gettimeofday(&s_time, NULL);

    OSMutexLocker locker(&fMutex);
    fCond.Wait(&fMutex, timeToWaitForPushArvd * 1000);

    gettimeofday(&e_time, NULL);

    long timeDiff = e_time.tv_sec * 10 + e_time.tv_usec / 100000 - s_time.tv_sec * 10 - s_time.tv_usec / 100000;
    //fprintf(stderr, "%ld %ld %ld %ld time diff: %ld\n\n", s_time.tv_sec, s_time.tv_usec, e_time.tv_sec, e_time.tv_usec, timeDiff);

    // assert timeToWaitForPush is 8s, if timeDiff > 7.8s, we regard it as time out.
    if (timeDiff > (timeToWaitForPushArvd * 10 - 2))
        return false;

    return true;
}

void PushInfo::notifyApp(void) {
    OSMutexLocker locker(&fMutex);
    fCond.Broadcast();
}

PushInfo::PushInfo() {
    isPushArrived = false;
}

PushInfo::~PushInfo() {
    delete fullUrl.Ptr;
    delete MQPayLoad.Ptr;
}

/*
 * rtsp://120.27.188.84:8888/realtime/1234/1/2/realtime.sdp
 * rtsp://120.27.188.84:8888/record/1234/1/12/20140820163420.sdp
 */
bool PushInfo::parsePushInfo(const StrPtrLen& src) {
    fullUrl.AllocateAndCopy(src);
    if (src.Ptr == NULL || src.Len < 1)
        return false;


    // record/1234/1/abc.sdp
    char* pos;

    if ((pos = fullUrl.FindNextChar('/', 7)) == NULL) return false;
    pos++;
    filePath.Set(pos, fullUrl.Len - (pos - fullUrl.Ptr));

    if (0 == strncmp(filePath.Ptr, "record", 6))
        isRealtime = false;
    else if (0 == strncmp(filePath.Ptr, "realtime", 8))
        isRealtime = true;
    else return false;

    // get / before 1234
    if ((pos = filePath.FindNextChar('/')) == NULL) return false;
    vehicleId.Ptr = ++pos;

    // get / before 1
    if ((pos = filePath.FindNextChar('/', pos)) == NULL) return false;
    vehicleId.Len = pos - vehicleId.Ptr;

    // left at least should great then "1/_.sdp"
    if (pos + 7 > filePath.GetEnd()) return false;

    if (*(++pos) == '0')
        isHD = true;
    else if (*pos == '1')
        isHD = false;
    else return false;
    if (*(++pos) != '/') return false;

    if (isRealtime) {
        cameraIndex.Ptr = ++pos;
        if ((pos = filePath.FindNextChar('/', pos)) == NULL) return false;
        cameraIndex.Len = pos - cameraIndex.Ptr;
        if (!NMSRTSPReqInfo::isDigital(cameraIndex)) return false;
    } else {
        startTime.Ptr = ++pos;
        if ((pos = filePath.FindNextChar('/', pos)) == NULL) return false;
        startTime.Len = pos - startTime.Ptr;
        if (!NMSRTSPReqInfo::isDigital(startTime)) return false;

        // filePath not include .sdp
        fileName.Set(++pos, filePath.GetEnd() - pos + 1 - 4);
    }

    return true;
}

void PushInfo::toMQPayLoad(const bool& isBegin) {
    if (MQPayLoad.Ptr == NULL) {
        char* strPayLoad = new char[maxPayLoadLen];
        memset(strPayLoad, 0, maxPayLoadLen);
        NMSRTSPReqInfo::strlcat(strPayLoad, "{\"ServiceType\":\"viedoPlayer\",\"Data_Type\":\"", maxPayLoadLen);

        if (isRealtime)
            NMSRTSPReqInfo::strlcat(strPayLoad, "Realtime", maxPayLoadLen);
        else
            NMSRTSPReqInfo::strlcat(strPayLoad, "Recording", maxPayLoadLen);

        NMSRTSPReqInfo::strlcat(strPayLoad, "\",\"URL\":\"", maxPayLoadLen);
        strncat(strPayLoad, fullUrl.Ptr, fullUrl.Len);
        NMSRTSPReqInfo::strlcat(strPayLoad, "\",\"VideoType\":\"", maxPayLoadLen);

        //if(isBegin) {   //this is unnecessary filed when Stop
        if (isHD)
            NMSRTSPReqInfo::strlcat(strPayLoad, "HD", maxPayLoadLen);
        else
            NMSRTSPReqInfo::strlcat(strPayLoad, "SD", maxPayLoadLen);
        //}

        if (isRealtime) {
            NMSRTSPReqInfo::strlcat(strPayLoad, "\",\"Index\":\"", maxPayLoadLen);
            strncat(strPayLoad, cameraIndex.Ptr, cameraIndex.Len);
        } else {
            NMSRTSPReqInfo::strlcat(strPayLoad, "\",\"CurrentTime\":\"", maxPayLoadLen);
            strncat(strPayLoad, startTime.Ptr, startTime.Len);
        }

        NMSRTSPReqInfo::strlcat(strPayLoad, "\",\"Operation\":\"", maxPayLoadLen);
        if (isBegin)
            NMSRTSPReqInfo::strlcat(strPayLoad, "Begin\"", maxPayLoadLen);
        else
            NMSRTSPReqInfo::strlcat(strPayLoad, "Stop\" ", maxPayLoadLen);

        NMSRTSPReqInfo::strlcat(strPayLoad, ",\"Datetime\":\"", maxPayLoadLen);
        struct timeval s_time;
        gettimeofday(&s_time, NULL);

        sprintf(strPayLoad + strlen(strPayLoad), "%ld\"}", ((long) s_time.tv_sec)*1000 + (long) s_time.tv_usec / 1000);
        MQPayLoad.Set(strPayLoad);
    } else { // MQ existing, we only need modify Operation.
        char* pos = MQPayLoad.FindString("Operation\":\"");
        if (pos == NULL) {
            fprintf(stderr, "[ERROR] PushInfo::toMQPayLoad can't find Operation in existing MQ.\n\n");
            return;
        }
        // ["Operation":"Begin"] ["Operation":"Stop" ]
        pos += strlen("Operation\":\"");
        if (isBegin && (0 == strncmp(pos, "Stop\" ", 6)))
            memcpy(pos, "Begin\"", 6);
        else if (!isBegin && (0 == strncmp(pos, "Begin\"", 6)))
            memcpy(pos, "Stop\" ", 6);
    }
}

bool PushInfo::PublishMq() const {
    char Topic[maxTopicLen] = {0};
    snprintf(Topic, maxTopicLen - 1, "/%.*s/videoinfoAsk", vehicleId.Len, vehicleId.Ptr);
    int rc = publishMq(strMQServerAddress, "EasyDarwin", Topic, MQPayLoad.Ptr, timeOutForSendMQ);
    if (0 != rc) {
        fprintf(stderr, "publishMq fail, return code: %d\n", rc);
        return false;
    }

    return true;
}

size_t NMSRTSPReqInfo::strlcat(char *dst, const char *src, size_t siz) {
    register char *d = dst;
    register const char *s = src;
    register size_t n = siz;
    size_t dlen;

    if (s == 0 || d == 0) return 0;

    /* Find the end of dst and adjust bytes left but don't go past end */
    while (n-- != 0 && *d != '\0')
        d++;
    dlen = d - dst;
    n = siz - dlen;

    if (n == 0)
        return (dlen + strlen(s));
    while (*s != '\0') {
        if (n != 1) {
            *d++ = *s;
            n--;
        }
        s++;
    }
    *d = '\0';

    return (dlen + (s - src)); /* count does not include NUL */
}

size_t NMSRTSPReqInfo::strlcpy(char *dst, const char *src, size_t siz) {
    char *d = dst;
    const char *s = src;
    size_t n = siz;

    /* Copy as many bytes as will fit */
    if (n != 0) {
        while (--n != 0) {
            if ((*d++ = *s++) == '\0')
                break;
        }
    }

    /* Not enough room in dst, add NUL and traverse rest of src */
    if (n == 0) {
        if (siz != 0)
            *d = '\0'; /* NUL-terminate dst */
        while (*s++)
            ;
    }

    return (s - src - 1); /* count does not include NUL */
}

bool NMSRTSPReqInfo::isDigital(StrPtrLen& src) {
    if (src.Len == 0 || src.Ptr == NULL)
        return false;

    for (int i = 0; src.Len > i; i++)
        if (('0' > src[i] || '9' < src[i]) && '.' != src[i])
            return false;

    return true;
}