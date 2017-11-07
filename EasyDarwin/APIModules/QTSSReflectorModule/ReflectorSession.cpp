/*
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * Copyright (c) 1999-2008 Apple Inc.  All Rights Reserved.
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_LICENSE_HEADER_END@
 *
 */
/*
        File:       ReflectorSession.cpp
        Contains:   Implementation of object defined in ReflectorSession.h.
 
 * Gavin's change:
 * in ReflectorSession::RemoveOutput, dected that there is no APP in session, 
 * generate url and call StopPushMQ.
 */

#include "ReflectorSession.h"
#include "SocketUtils.h"

#include "OSMemory.h"
#include "OS.h"
#include "atomic.h"

#include "QTSServerInterface.h"
#include <errno.h>
#include "sdpCache.h"

#ifndef __Win32__
#include <unistd.h>
#endif

#if 0
#include "mainProcess.h"
extern const int timeOutForSendMQ;
extern int ServerPort;
extern char ServerIP[20];
#endif
#include "RTSPReqInfo.h"

#if DEBUG
#define REFLECTOR_SESSION_DEBUGGING 0
#else
#define REFLECTOR_SESSION_DEBUGGING 0
#endif


FileDeleter::FileDeleter(StrPtrLen* inSDPPath) {
    Assert(inSDPPath);
    fFilePath.Len = inSDPPath->Len;
    fFilePath.Ptr = NEW char[inSDPPath->Len + 1];
    Assert(fFilePath.Ptr);
    memcpy(fFilePath.Ptr, inSDPPath->Ptr, inSDPPath->Len);
    fFilePath.Ptr[inSDPPath->Len] = 0;
}

FileDeleter::~FileDeleter() {
    //qtss_printf("FileDeleter::~FileDeleter delete = %s \n",fFilePath.Ptr);
    ::unlink(fFilePath.Ptr);
    delete fFilePath.Ptr;
    fFilePath.Ptr = NULL;
    fFilePath.Len = 0;
}

void ReflectorSession::Initialize() {
    ;
}

ReflectorSession::ReflectorSession(StrPtrLen* inSourceID, SourceInfo* inInfo) :
fIsSetup(false),
fQueueElem(),
fNumOutputs(0),
fStreamArray(NULL),
fFormatter(fHTMLBuf, kMaxHTMLSize),
fSourceInfo(inInfo),
fSocketStream(NULL),
fBroadcasterSession(NULL),
fInitTimeMS(OS::Milliseconds()),
fHasBufferedStreams(false),
fRTSPRelaySession(NULL),
fSessionName(NULL),
fHLSLive(false),
fHasVideoKeyFrameUpdate(false),
fStreamName(NULL) {

    fQueueElem.SetEnclosingObject(this);
    if (inSourceID != NULL) {
        fSourceID.Ptr = NEW char[inSourceID->Len + 1];
        ::memcpy(fSourceID.Ptr, inSourceID->Ptr, inSourceID->Len);
        fSourceID.Ptr[inSourceID->Len] = '\0';
        fSourceID.Len = inSourceID->Len;
        fRef.Set(fSourceID, this);

        this->SetSessionName();
    }
}

ReflectorSession::~ReflectorSession() {
    this->StopHLSSession();

#if REFLECTOR_SESSION_DEBUGGING
    qtss_printf("Removing ReflectorSession: %s\n", fSourceInfoHTML.Ptr);
#endif

    // For each stream, check to see if the ReflectorStream should be deleted
    for (UInt32 x = 0; x < fSourceInfo->GetNumStreams(); x++) {
        if (fStreamArray[x] == NULL)
            continue;

        fStreamArray[x]->SetMyReflectorSession(NULL);

        delete fStreamArray[x];
        fStreamArray[x] = NULL;
    }

    CSdpCache::GetInstance()->eraseSdpMap(GetSourcePath()->GetAsCString());

    // We own this object when it is given to us, so delete it now
    delete[] fStreamArray;
    delete fSourceInfo;
    fLocalSDP.Delete();
    fSourceID.Delete();

    //将推流名称从redis中删除,使用fSessionName+".sdp"
    if (fStreamName) {
        QTSS_RoleParams theParams;
        theParams.StreamNameParams.inStreamName = fStreamName;
        UInt32 numModules = QTSServerInterface::GetNumModulesInRole(QTSSModule::kRedisDelPushStreamRole);
        for (UInt32 currentModule = 0; currentModule < numModules; currentModule++) {
            qtss_printf("从redis中删除推流名称%s\n", fStreamName);
            QTSSModule* theModule = QTSServerInterface::GetModule(QTSSModule::kRedisDelPushStreamRole, currentModule);
            (void) theModule->CallDispatch(Easy_RedisDelPushStream_Role, &theParams);
        }
    }
    if (fSessionName)
        delete[] fSessionName;
}

QTSS_Error ReflectorSession::SetSessionName() {
    if (fSourceID.Len > 0) {
        char movieFolder[256] = {0};
        UInt32 thePathLen = 256;
        QTSServerInterface::GetServer()->GetPrefs()->GetMovieFolder(&movieFolder[0], &thePathLen);
        StringParser parser(&fSourceID);
        StrPtrLen strName;

        parser.ConsumeLength(NULL, thePathLen);

        parser.Expect('\\');
        parser.ConsumeUntil(&strName, '\.');
        fSessionName = NEW char[strName.Len + 1];
        ::memcpy(fSessionName, strName.Ptr, strName.Len);
        fSessionName[strName.Len] = '\0';

        //将推流名称加入到redis中
        int iLen = strlen(fSessionName) + 1;
        fStreamName = new char[iLen];
        sprintf(fStreamName, "%s", fSessionName);
        for (int i = 0; i < iLen; i++) {
            if (fStreamName[i] == '\\')
                fStreamName[i] = '/';
        }
        QTSS_RoleParams theParams;
        theParams.StreamNameParams.inStreamName = fStreamName;
        UInt32 numModules = QTSServerInterface::GetNumModulesInRole(QTSSModule::kRedisAddPushStreamRole);
        for (UInt32 currentModule = 0; currentModule < numModules; currentModule++) {
            qtss_printf("向redis中添加推流名称%s\n", fStreamName);

            QTSSModule* theModule = QTSServerInterface::GetModule(QTSSModule::kRedisAddPushStreamRole, currentModule);
            (void) theModule->CallDispatch(Easy_RedisAddPushStream_Role, &theParams);
        }
        return QTSS_NoErr;
    }
    return QTSS_Unimplemented;
}

QTSS_Error ReflectorSession::StartHLSSession() {
    QTSS_Error theErr = QTSS_NoErr;

    if (!fHLSLive) {
        // Get the ip addr out of the prefs dictionary
        UInt16 thePort = 554;
        UInt32 theLen = sizeof (UInt16);
        theErr = QTSServerInterface::GetServer()->GetPrefs()->GetValue(qtssPrefsRTSPPorts, 0, &thePort, &theLen);
        Assert(theErr == QTSS_NoErr);

        //构造本地URL
        char url[QTSS_MAX_URL_LENGTH] = {0};
        qtss_sprintf(url, "rtsp://127.0.0.1:%d/%s.sdp", thePort, fSessionName);

        Easy_StartHLSession(fSessionName, url, 0, NULL);
        //if(QTSS_NoErr == theErr)
        fHLSLive = true;
    }

    return theErr;
}

QTSS_Error ReflectorSession::StopHLSSession() {
    QTSS_Error theErr = QTSS_NoErr;

    if (fHLSLive) {
        theErr = Easy_StopHLSession(fSessionName);
        if (QTSS_NoErr == theErr)
            fHLSLive = false;
    }

    return theErr;
}

QTSS_Error ReflectorSession::SetupReflectorSession(SourceInfo* inInfo, QTSS_StandardRTSP_Params* inParams, UInt32 inFlags, Bool16 filterState, UInt32 filterTimeout) {
    // use the current SourceInfo
    if (inInfo == NULL)
        inInfo = fSourceInfo;

    // Store a reference to this sourceInfo permanently
    Assert((fSourceInfo == NULL) || (inInfo == fSourceInfo));
    fSourceInfo = inInfo;

    // this must be set to the new SDP.
    fLocalSDP.Delete();
    fLocalSDP.Ptr = inInfo->GetLocalSDP(&fLocalSDP.Len);

    // 全部重新构造ReflectorStream
    if (fStreamArray != NULL) {
        delete fStreamArray; // keep the array list synchronized with the source info.
    }

    fStreamArray = NEW ReflectorStream*[fSourceInfo->GetNumStreams()];
    ::memset(fStreamArray, 0, fSourceInfo->GetNumStreams() * sizeof (ReflectorStream*));

    for (UInt32 x = 0; x < fSourceInfo->GetNumStreams(); x++) {
        fStreamArray[x] = NEW ReflectorStream(fSourceInfo->GetStreamInfo(x));
        // Obviously, we may encounter an error binding the reflector sockets.
        // If that happens, we'll just abort here, which will leave the ReflectorStream
        // array in an inconsistent state, so we need to make sure in our cleanup
        // code to check for NULL.
        QTSS_Error theError = fStreamArray[x]->BindSockets(inParams, inFlags, filterState, filterTimeout);
        if (theError != QTSS_NoErr) {
            delete fStreamArray[x];
            fStreamArray[x] = NULL;
            return theError;
        }
        fStreamArray[x]->SetMyReflectorSession(this);

        fStreamArray[x]->SetEnableBuffer(this->fHasBufferedStreams); // buffering is done by the stream's sender

        // If the port was 0, update it to reflect what the actual RTP port is.
        fSourceInfo->GetStreamInfo(x)->fPort = fStreamArray[x]->GetStreamInfo()->fPort;
        //qtss_printf("ReflectorSession::SetupReflectorSession fSourceInfo->GetStreamInfo(x)->fPort= %u\n",fSourceInfo->GetStreamInfo(x)->fPort);   
    }

    if (inFlags & kMarkSetup)
        fIsSetup = true;

    return QTSS_NoErr;
}

void ReflectorSession::AddBroadcasterClientSession(QTSS_StandardRTSP_Params* inParams) {
    if (NULL == fStreamArray || NULL == inParams)
        return;

    for (UInt32 x = 0; x < fSourceInfo->GetNumStreams(); x++) {
        if (fStreamArray[x] != NULL) { //qtss_printf("AddBroadcasterSession=%"   _U32BITARG_   "\n",inParams->inClientSession);
            ((ReflectorSocket*) fStreamArray[x]->GetSocketPair()->GetSocketA())->AddBroadcasterSession(inParams->inClientSession);
            ((ReflectorSocket*) fStreamArray[x]->GetSocketPair()->GetSocketB())->AddBroadcasterSession(inParams->inClientSession);
        }
    }
    fBroadcasterSession = inParams->inClientSession;
}

void ReflectorSession::FormatHTML(StrPtrLen* inURL) {
    // Begin writing our source description HTML (used by the relay)
    // Line looks like: Relay Source: 17.221.98.239, Ports: 5430 5432 5434
    static StrPtrLen sHTMLStart("<H2>Relay Source: ");
    static StrPtrLen sPorts(", Ports: ");
    static StrPtrLen sHTMLEnd("</H2><BR>");

    // Begin writing the HTML
    fFormatter.Put(sHTMLStart);

    if (inURL == NULL) {
        // If no URL is provided, format the source IP addr as a string.
        char theIPAddrBuf[20];
        StrPtrLen theIPAddr(theIPAddrBuf, 20);
        struct in_addr theAddr;
        theAddr.s_addr = htonl(fSourceInfo->GetStreamInfo(0)->fSrcIPAddr);
        SocketUtils::ConvertAddrToString(theAddr, &theIPAddr);
        fFormatter.Put(theIPAddr);
    } else
        fFormatter.Put(*inURL);

    fFormatter.Put(sPorts);

    for (UInt32 x = 0; x < fSourceInfo->GetNumStreams(); x++) {
        fFormatter.Put(fSourceInfo->GetStreamInfo(x)->fPort);
        fFormatter.PutSpace();
    }
    fFormatter.Put(sHTMLEnd);

    // Setup the StrPtrLen to point to the right stuff
    fSourceInfoHTML.Ptr = fFormatter.GetBufPtr();
    fSourceInfoHTML.Len = fFormatter.GetCurrentOffset();

    fFormatter.PutTerminator();
}

void ReflectorSession::AddOutput(ReflectorOutput* inOutput, Bool16 isClient) {
    Assert(fSourceInfo->GetNumStreams() > 0);

    // We need to make sure that this output goes into the same bucket for each ReflectorStream.
    SInt32 bucket = -1;
    SInt32 lastBucket = -1;

    while (true) {
        UInt32 x = 0;
        for (; x < fSourceInfo->GetNumStreams(); x++) {
            bucket = fStreamArray[x]->AddOutput(inOutput, bucket);
            if (bucket == -1) // If this output couldn't be added to this bucket,
                break; // break and try again
            else {
                lastBucket = bucket; // Remember the last successful bucket placement.
                if (isClient)
                    fStreamArray[x]->IncEyeCount();
            }
        }

        if (bucket == -1) {
            // If there was some kind of conflict adding this output to this bucket,
            // we need to remove it from the streams to which it was added.
            for (UInt32 y = 0; y < x; y++) {
                fStreamArray[y]->RemoveOutput(inOutput);
                if (isClient)
                    fStreamArray[y]->DecEyeCount();
            }

            // Because there was an error, we need to start the whole process over again,
            // this time starting from a higher bucket
            lastBucket = bucket = lastBucket + 1;
        } else
            break;
    }
    (void) atomic_add(&fNumOutputs, 1);
}

void ReflectorSession::RemoveOutput(ReflectorOutput* inOutput, Bool16 isClient) {
    (void) atomic_sub(&fNumOutputs, 1);
    for (UInt32 y = 0; y < fSourceInfo->GetNumStreams(); y++) {
        fStreamArray[y]->RemoveOutput(inOutput);
        if (isClient)
            fStreamArray[y]->DecEyeCount();
    }

    //移除客户端之后判断fNumOutputs是否为0,add
    if (fNumOutputs == 0) {
        //调用角色，停止推流
        QTSS_RoleParams theParams;
        theParams.easyFreeStreamParams.inStreamName = fStreamName;
        UInt32 currentModule = 0;
        UInt32 numModules = QTSServerInterface::GetNumModulesInRole(QTSSModule::kEasyCMSFreeStreamRole);
        for (; currentModule < numModules; currentModule++) {
            QTSSModule* theModule = QTSServerInterface::GetModule(QTSSModule::kEasyCMSFreeStreamRole, currentModule);
            (void) theModule->CallDispatch(Easy_CMSFreeStream_Role, &theParams);
            break;
        }


        // fStreamName is like "realtime/$1234/1/realtime"
        // we need splice tobe realtime/$1234/1/realtim.sdp    
        DateBuffer theDate;
        int lenOffStreamName = strlen(fStreamName);
        char* cFullFileName = new char[5 + lenOffStreamName];
        sprintf(cFullFileName, "%s.sdp", fStreamName);
        *(cFullFileName + 5 + lenOffStreamName - 1) = 0;
        DateTranslator::UpdateDateBuffer(&theDate, 0);
        fprintf(stderr, "[INFO] %s: ReflectorSession::%s: fNumOutputs == 0. %s TID: %lu\n\n", cFullFileName, __func__, theDate.GetDateBuffer(), OSThread::GetCurrentThreadID());
        UnRegisterAndSendMQAndDelete(cFullFileName);
        delete cFullFileName;
    }
}

void ReflectorSession::TearDownAllOutputs() {
    for (UInt32 y = 0; y < fSourceInfo->GetNumStreams(); y++)
        fStreamArray[y]->TearDownAllOutputs();
}

void ReflectorSession::RemoveSessionFromOutput(QTSS_ClientSessionObject inSession) {
    for (UInt32 x = 0; x < fSourceInfo->GetNumStreams(); x++) {
        ((ReflectorSocket*) fStreamArray[x]->GetSocketPair()->GetSocketA())->RemoveBroadcasterSession(inSession);
        ((ReflectorSocket*) fStreamArray[x]->GetSocketPair()->GetSocketB())->RemoveBroadcasterSession(inSession);
    }
    fBroadcasterSession = NULL;
}

UInt32 ReflectorSession::GetBitRate() {
    UInt32 retval = 0;
    for (UInt32 x = 0; x < fSourceInfo->GetNumStreams(); x++)
        retval += fStreamArray[x]->GetBitRate();
    return retval;
}

Bool16 ReflectorSession::Equal(SourceInfo* inInfo) {
    return fSourceInfo->Equal(inInfo);
}

void* ReflectorSession::GetStreamCookie(UInt32 inStreamID) {
    for (UInt32 x = 0; x < fSourceInfo->GetNumStreams(); x++) {
        if (fSourceInfo->GetStreamInfo(x)->fTrackID == inStreamID)
            return fStreamArray[x]->GetStreamCookie();
    }
    return NULL;
}