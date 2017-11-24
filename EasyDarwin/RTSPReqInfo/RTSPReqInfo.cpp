/**
 * bug1����app1�ڲ��ų���A��¼��1ʱ��app2���󲥷ų���A��¼��2��8s��ʱȻ����stopMQ������A�������ͻ�ֹͣ������app1Ҳ���Ž���
 * solution1: �����˽��յ�stopMQʱ��Ӧ�ж� full url �Ƿ��뵱ǰ���ڲ��ŵ�urlһ�£�һ����ֹͣ��������ԡ�
 * solution2: ���������key�� filePath ���� vehicleId ,app2 option���󵽴�ʱ���ֳ���A��ע�ᣬ�����ߣ�
 * Ȼ��desc404����ʱ���ж�ע���������Ϣ�е�filePath�Ƿ��뵱ǰ����filePathһ�£���һ������ܳ�������̫����������ƣ�
 * ��Ҫ����stopMQ�����������A����������Ƶ��������MQ
 * 
 * bug2: ��vlc���Ż�������⣺
 * �����ǵ��Լ�����������ʱû�г�ʱ����8s��û�յ���������������option���߳�������Ȼ��rtsp���̼�����app��desc��ED����desc404�����ص���
 * UnRegisterAndSendMQAndDelete() ����stopMQȻ����ע���rtspReqInfoTable��ɾ������ɾ��pushInfoʵ����
 * ��vlc�Լ��г�ʱʱ�䣬�����4s����4s��û���յ���Ƶ���ݣ�vlc�����ٷ��κ�RTSP�����ED������pushInfoʵ��û��ɾ����ע���rtspReqInfoTable
 * ��Ҳû��ɾ�����´��������ʱ��ע���rtspReqInfoTable�Ѵ��ڣ�����Ϊ��ǰ����BeginMQ�Ѿ����������ٷ��͡�
 * solution: ��Ҫ����������������ͻ����Դ���ʱ������Ϊ��
 * case1 ����û����������Ƶ��app option timeout��Ȼ�����stopMQ,Ȼ��ɾ��pushInfoʵ��
 * case2 ����������������Ƶ������bug1����ǰapp option ����������404��������app�Դ���ʱ���ص����������
 * 
 * bug3: Easydarwin����app describe��motor setup �̵߳���˳������
 * app���������������appOption�̣߳���ʱ��������������option����Ȼ�����������̱߳����Ѻ��Ѿ�usleep(1000 * 500)��
 * ���п���app describe��motor setup�类������ʱ������������ﵫ�ǲ���ʧ�ܵ����γ���
 * solution: ��RTSPRequestInterface.cpp�з���desc 404�ĵط�����UnRegisterAndSendMQAndDelete�ر�������
 * 
 * req1:
 * ¼���϶���filePath ��Ƶ��ʼ����ʱ��仯���������䡣�����0s�����50s��app option��Ҫ�ȵ���һ������0s
 * ����Ƶ��·�������ټ���������һ�����󣬼�50s��
 * solution1: ��������
 *  �ȴ�������rtspReqInfoTable�д��ڳ�����ʼ����ʱ�䲻ͬ��filePath
 *  �����������������������ID teardown��ʱ��
 *  ��������������ID
 * solution2: ���϶���app option��ǿ��sleep
 * 
 * ���Ϸ��������У���Ϊ��ʹ�ܱ�֤�����Ĳ�����������һ����ɺ�solution2��ʵ�֣���֮������̣�
 * ����option����app option,����motor setup �ܴ������app desc֮����ʧ�ܣ�����ͬһ���̣߳�
 * ������app�������϶���ǿ��sleep����ʱ�����޸�
 */



#include "RTSPReqInfo.h"
#include "QTSServerInterface.h"
#include <sys/time.h>

#include <string>
#include <chrono>
#include <ctime>
#include "mainProcess.h"
#include "cerr.h"

/**
 * ��PUSH_STOP_COMFIRM ���أ����ɿ���ȷ�������رջ��ƣ�������StopMQ�����̵߳ȴ������رջ��µ��������󣬳�ʱ����Ϊ����δ�رգ�BUG��
 */
#ifdef PUSH_STOP_COMFIRM
#include <map>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <memory>
#endif

#define TIMEFORMAT "%Y-%m-%d %H:%M:%S"
using namespace std;

#ifdef PUSH_STOP_COMFIRM
typedef struct lock {
    mutex mtx; // ȫ�ֻ�����.
    condition_variable cv; // ȫ����������.   
} lock_t;
typedef map<string, std::shared_ptr<lock_t>> lock_tbl_t;
#endif

const char *strClientIdForMQ = "EasyDarwin";
const char *strMQServerAddress = "ssl://120.26.86.124:8883";

const int CTimeToWaitForPushArvd = 8;
const int timeOutForSendMQ = 4;
const char *strCarUserAgentValue = "LeapMotor Push";
char *strUserAgentKeyWord = "User-Agent:";
const int maxPayLoadLen = 2000;
const int maxTopicLen = 500;

#ifdef PUSH_STOP_COMFIRM
static lock_tbl_t s_lock_tbl;
static mutex lock_tbl_mutex;
#endif

static OSRefTable* s_rtspReqInfoTable = nullptr;
OSMutex* PushInfo::fMutexForSendMQ = NULL;
namespace NMSRTSPReqInfo {
    size_t strlcat(char *dst, const char *src, size_t siz);
    size_t strlcpy(char *dst, const char *src, size_t siz);
    bool isDigital(StrPtrLen& src);
}

string time_str(const time_t& time) {
    struct tm* timeTM;
    char strTime[22] = {0};

    timeTM = localtime(&time);
    strftime(strTime, sizeof (strTime) - 1, TIMEFORMAT, timeTM);
    return string(strTime);
}

string now_str() {
    return time_str(time(NULL));
}

#ifdef PUSH_STOP_COMFIRM
shared_ptr<lock_t> get_lock(const string& key) {
    unique_lock<mutex> lk(lock_tbl_mutex);
    lock_tbl_t::iterator it;
    if (s_lock_tbl.end() == (it = s_lock_tbl.find(key))) {
        shared_ptr<lock_t> lock = make_shared<lock_t>();
        return s_lock_tbl.insert(pair<string, shared_ptr < lock_t >> (key, lock)).first->second;
    }

    return it->second;
}

bool wait(const string& vehicle, const size_t& second) {
    shared_ptr<lock_t> lock = get_lock(vehicle);
    unique_lock<mutex> lk(lock->mtx);
    return lock->cv.wait_for(lk, chrono::seconds(second)) == cv_status::timeout;
}

void notify(const string& vehicle) {
    shared_ptr<lock_t> lock = get_lock(vehicle);
    unique_lock<mutex> lk(lock->mtx);
    lock->cv.notify_all();
    cerr << "[DEBUG] " << vehicle << " notified " << now_str() << "\n\n";
}

void notify(const char* vehicle) {
    notify(string(vehicle));
}

void notify(const StrPtrLen& vehicle) {
    notify(string(vehicle.Ptr, vehicle.Len));
}

void listen_push_status(const string& vehicle) {
    cerr << "[DEBUG] new thread start to wait for " << vehicle << " stop " << now_str() << "\n\n";
    if (wait(vehicle, 10)) {
        cerr << "[ERROR] " << vehicle << ": stop MQ sent but vehicle didn't stop pushing "
                << now_str() << "\n\n";
    } else
        cerr << "[DEBUG] " << vehicle << ": stop push successful " << now_str() << "\n\n";
}
#endif

// parseAndRegisterAndSendBeginMQAndWait
void on_rtsp_request(const StrPtrLen& req) {
    if (!s_rtspReqInfoTable)
        s_rtspReqInfoTable = QTSServerInterface::GetServer()->GetRTSPReqInfoMap();

    if (NULL == req.Ptr || req.Len < 1) {
        fprintf(stderr, "[ERROR] %s: req invalid.\n\n", __func__);
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
//    bool MotorAnnounce = rtspReqInfo.isFromLeapMotor && rtspReqInfo.RTSPType == announce;
    bool AppOption = !rtspReqInfo.isFromLeapMotor && rtspReqInfo.RTSPType == option;
    bool MotorTeardown = rtspReqInfo.isFromLeapMotor && rtspReqInfo.RTSPType == teardown;

    if (MotorOption || AppOption || MotorTeardown) {

        pushInfoRef = s_rtspReqInfoTable->Resolve(&rtspReqInfo.vehicleId);
        
        if (NULL == pushInfoRef) { // ��ǰ����û����������������֮ǰû������app�ڵȴ���ǰ��������
            if (MotorTeardown)
                return;
            if (MotorOption) {
                fprintf(stderr, "[WARN] %.*s: MotorOption arrived, but there's no app wait for push. %s TID: %lu\n\n",
                        rtspReqInfo.filePath.Len, rtspReqInfo.filePath.Ptr, now_str().c_str(), OSThread::GetCurrentThreadID());
            }
            pushInfo = new PushInfo();
            if (!pushInfo->parsePushInfo(rtspReqInfo.fullUrl)) {
                
                fprintf(stderr, "[ERROR] %.*s: pushInfo->parsePushInfo fail. %s TID: %lu\n\n",
                        rtspReqInfo.filePath.Len, rtspReqInfo.filePath.Ptr, now_str().c_str(), OSThread::GetCurrentThreadID());
                delete pushInfo;
                return;
            }

            pushInfo->readyToAddToTable();
            if (OS_NoErr != s_rtspReqInfoTable->Register(pushInfo->GetRef())) {
                
                fprintf(stderr, "[ERROR] %.*s: Register fail. %s TID: %lu\n\n",
                        rtspReqInfo.filePath.Len, rtspReqInfo.filePath.Ptr, now_str().c_str(), OSThread::GetCurrentThreadID());
                delete pushInfo;
                return;
            }
            
            fprintf(stderr, "[INFO] %.*s: PushInfo allocated & registered. %s TID: %lu\n\n",
                    rtspReqInfo.filePath.Len, rtspReqInfo.filePath.Ptr, now_str().c_str(), OSThread::GetCurrentThreadID());

        } else { // ������Ϣ�Ѵ��� may another app with same url is waiting, may push has arrived.
            pushInfo = (PushInfo*) pushInfoRef->GetObject();
            if (AppOption) {
                fprintf(stderr, "[DEBUG] %.*s: PushInfo is existing. %s TID: %lu\n\n",
                        rtspReqInfo.filePath.Len, rtspReqInfo.filePath.Ptr, now_str().c_str(), OSThread::GetCurrentThreadID());
            } else if (MotorTeardown) { // һ�㶼���ڷ���stop mq�Ժ�ɾ��pushInfo�����Ե�����˵���ǳ��������Ͽ�

                s_rtspReqInfoTable->UnRegister(pushInfoRef, 0xffffffff);
                delete pushInfo;
                
                fprintf(stderr, "[INFO] %.*s: vehicle teardown by it self, pushInfo unregistered and deleted. %s TID: %lu\n\n",
                        rtspReqInfo.filePath.Len, rtspReqInfo.filePath.Ptr, now_str().c_str(), OSThread::GetCurrentThreadID());
                return;
            }
        }

        
        if (MotorOption) {
            pushInfo->isPushArrived = true;
            fprintf(stderr, "[DEBUG] %.*s: pushInfo->isPushArrived set true, notifyAppThread wake up. %s TID: %lu\n\n",
                    pushInfo->filePath.Len, pushInfo->filePath.Ptr, now_str().c_str(), OSThread::GetCurrentThreadID());
            pushInfo->notifyApp();
        } else if (!pushInfo->isPushArrived) { // app option && push hasn't arrived
            if (NULL == pushInfoRef) { // δע��ŷ���BeginMQ
                pushInfo->sendBeginOrStopMq(true);
#ifdef PUSH_STOP_COMFIRM
                notify(string(rtspReqInfo.vehicleId.Ptr, rtspReqInfo.vehicleId.Len));
#endif
                fprintf(stderr, "[DEBUG] %.*s: AppOption & Push hasn't Arrived & hasn't registered, begin MQ sent. %s TID: %lu\n\n",
                        pushInfo->filePath.Len, pushInfo->filePath.Ptr, now_str().c_str(), OSThread::GetCurrentThreadID());
            } else // ��ע�ᵫ����δ����ʱ���ٷ���BeginMQ
                fprintf(stderr, "[DEBUG] %.*s: AppOption & Push hasn't Arrived & registered. %s TID: %lu\n\n", pushInfo->filePath.Len, pushInfo->filePath.Ptr, now_str().c_str(), OSThread::GetCurrentThreadID());

            if (pushInfo->waitForNotification(CTimeToWaitForPushArvd)) {
                usleep(1000 * 500); // at this time, motor and app are all in option, we delay app to let motor setup first.
            } else {
                

                /*
                 * for bug2, ��Ϊ���app�Դ���ʱ��ǰ���أ��������desc404�����̣�������Ҫ��������ͷ���Դ��
                 * ��app����ǰ���أ���������Դ���ͷţ��ڽ���desc404�󣬷������filePathδ��ע�ᣬdo nothing
                 */
                fprintf(stderr, "[INFO] %.*s: Wait for push timeout(%ds). %s TID: %lu\n\n",
                        rtspReqInfo.filePath.Len, rtspReqInfo.filePath.Ptr, CTimeToWaitForPushArvd, now_str().c_str(), OSThread::GetCurrentThreadID());
                // in case that 2 apps wait for a same url push but timeout, first one call UnRegister the url and another call again, Resolve will fail.                    
                if (NULL == s_rtspReqInfoTable->ResolveAndUnRegister(&rtspReqInfo.vehicleId)) {
                    fprintf(stderr, "[INFO] %.*s: PushInfo already deleted, nothing to do. %s TID: %lu\n\n",
                            rtspReqInfo.filePath.Len, rtspReqInfo.filePath.Ptr, now_str().c_str(), OSThread::GetCurrentThreadID());
                    return;
                }
                pushInfo->sendBeginOrStopMq(false);

#ifdef PUSH_STOP_COMFIRM
                thread t(listen_push_status, string(rtspReqInfo.vehicleId.Ptr, rtspReqInfo.vehicleId.Len));
                t.detach();
#endif
                delete pushInfo;
                
                fprintf(stderr, "[INFO] %.*s: PushInfo unregistered and deleted, stop MQ sent. %s TID: %lu\n\n",
                        rtspReqInfo.filePath.Len, rtspReqInfo.filePath.Ptr, now_str().c_str(), OSThread::GetCurrentThreadID());
            }
        } else { // AppOption && PushArrived
            fprintf(stderr, "[DEBUG] %.*s: AppOption & PushArrived, app option thread will continue. %s TID: %lu\n\n",
                    rtspReqInfo.filePath.Len, rtspReqInfo.filePath.Ptr, now_str().c_str(), OSThread::GetCurrentThreadID());
        }
    }

}

/**
 * 
 * @param key expected to be like "realtime/$1234/1/realtim.sdp"
 * @param call_from_describe
 */
void UnRegisterAndSendMQAndDelete(char *key, bool call_from_describe/* = false*/) {

    if (!s_rtspReqInfoTable)
        s_rtspReqInfoTable = QTSServerInterface::GetServer()->GetRTSPReqInfoMap();

    StrPtrLen fullFileName(key);
    StrPtrLen vehicleId;
    vehicleId.Ptr = fullFileName.FindNextChar('/') + 1;
    vehicleId.Len = fullFileName.FindNextChar('/', vehicleId.Ptr) - vehicleId.Ptr;
    PushInfo* pushInfo;
    OSRef* pushInfoRef = s_rtspReqInfoTable->Resolve(&vehicleId);
    
    if (NULL == pushInfoRef) {
        
        /**
         * ��ReflectorSession.cpp�м�⵽û��app����������ʱ�ͷ���desc 404(app����ʱ��app����ʱ������������һ·��Ƶ��bug3)
         * ʱ����ô˺������������pushInfo��Ӧ�û���
         * �����г����ȶϿ����ӣ���ʱpushInfo�Ѿ���ɾ��
         */
        fprintf(stderr, "[INFO] %s: pushInfo hasn't registered when try to UnRegister. %s TID: %lu\n\n", key, now_str().c_str(), OSThread::GetCurrentThreadID());
        return;
    }
    pushInfo = (PushInfo*) pushInfoRef->GetObject();

    // for bug1
    if (call_from_describe && !pushInfo->filePath.Equal(fullFileName)) {
        
        fprintf(stderr, "[DEBUG] %s: This vehicle is pushing another url: %.*s. Nothing to do. %s TID: %lu\n\n",
                key, pushInfo->filePath.Len, pushInfo->filePath.Ptr, now_str().c_str(), OSThread::GetCurrentThreadID());
        return;
    }
    s_rtspReqInfoTable->UnRegister(pushInfoRef, 0xffffffff);
    pushInfo->sendBeginOrStopMq(false);

#ifdef PUSH_STOP_COMFIRM
    thread t(listen_push_status, string(vehicleId.Ptr, vehicleId.Len));
    t.detach();
#endif
    delete pushInfo;
    
    fprintf(stderr, "[DEBUG] %s: PushInfo unregistered and deleted, stop MQ sent. %s TID: %lu\n\n",
            key, now_str().c_str(), OSThread::GetCurrentThreadID());
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
        
        fprintf(stderr, "************ %.6s %.9s %s TID: %lu\n\n",
                completeRequest.Ptr, pos, now_str().c_str(), OSThread::GetCurrentThreadID());
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

    if (!PublishMq() && !PublishMq()) // ����ط�����
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
