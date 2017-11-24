#ifndef RTSPREQINFOTABLE_H
#define RTSPREQINFOTABLE_H

#include <map>
//#include <string>
#include "StrPtrLen.h"
#include <string.h>
#include "RTSPReqInfo.h"

//typedef std::map<int, RTSPReqInfo*> mapRTSPReqTable;

/*
 * we only Register app's pull request in this table
 */
class RTSPReqInfoTable {
public:
    RTSPReqInfoTable();
    RTSPReqInfoTable(const RTSPReqInfoTable& orig);
    virtual ~RTSPReqInfoTable();
    
    /*
     * rc:
     * true for insert successful
     * false for key already existed
     */
    bool Register(RTSPReqInfo* rtspReqInfo);
    
   
    /*
     * when return desc 404 in RTSPRequestInterface.cpp, or detect there is no app online in ReflectorSession call this function.
     * rc:
     * true for successful.
     * false for fail.
     */
    bool UnRegister(const StrPtrLen& fullFileName);
    
    /*
     * return NULL if not existing.
     */
    RTSPReqInfo* GetRTSPReqInfoByKey(const StrPtrLen& fullFileName);

    
private:
    //mapRTSPReqTable RTSPInfoTable;
    //mapRTSPReqTable::iterator it;
    
};

#endif /* RTSPREQINFOTABLE_H */

