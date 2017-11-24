#include "RTSPReqInfoTable.h"

RTSPReqInfoTable::RTSPReqInfoTable() {
}

RTSPReqInfoTable::RTSPReqInfoTable(const RTSPReqInfoTable& orig) {
}

RTSPReqInfoTable::~RTSPReqInfoTable() {
}

/*
 * rc:
 * 0 for insert successful
 * 1 for key already existed
 */
bool RTSPReqInfoTable::Register(RTSPReqInfo* rtspReqInfo) {
    //MtxForTable.lock();
//    std::string key(rtspReqInfo->fullFileName.Ptr, rtspReqInfo->fullFileName.Len);
//    if (RTSPInfoTable.end() == (it = RTSPInfoTable.find(key))){
//        RTSPInfoTable.insert(std::pair<std::string, RTSPReqInfo*>(key, rtspReqInfo));
//        //MtxForTable.unlock();
//        return true;
//    }
    //MtxForTable.unlock();
    return false;
}

/*
 * rc:
 * 0 for delete successful.
 * 1 for key not existing.
 */
bool RTSPReqInfoTable::UnRegister(const StrPtrLen& fullFileName) {
    //MtxForTable.lock();
//    std::string key(fullFileName.Ptr, fullFileName.Len);
//    if (RTSPInfoTable.end() == (it = RTSPInfoTable.find(key))){        
//        //MtxForTable.unlock();
//        return false;
//    }    
//    RTSPInfoTable.erase(it);
//    //MtxForTable.unlock();
    return true;
}

/*
 * rc:
 * NULL for not existing.
 */
RTSPReqInfo* RTSPReqInfoTable::GetRTSPReqInfoByKey(const StrPtrLen& fullFileName) {
    //MtxForTable.lock();
//    std::string key(fullFileName.Ptr, fullFileName.Len);
//    if (RTSPInfoTable.end() == (it = RTSPInfoTable.find(key))){
//        //MtxForTable.unlock();
//        return NULL;
//    }    
//    //MtxForTable.unlock();
    return it->second;    
}