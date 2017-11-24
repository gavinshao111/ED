/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   RTPSessionSaveOutput.h
 * Author: Gavin
 *
 * Created on 2016/10/20, 16:16
 */

#ifndef RTPSESSIONSAVEOUTPUT_H
#define RTPSESSIONSAVEOUTPUT_H

#include "ReflectorOutput.h"
#include "ReflectorSession.h"
#include "QTSS.h"

#define MAXPATHLEN 256

class RTPSessionSaveOutput: public ReflectorOutput  
{  
public:  
  
        RTPSessionSaveOutput(ReflectorSession* inReflectorSession, char* theFileName = NULL);  
        virtual ~RTPSessionSaveOutput();  
          

        QTSS_Error  WritePacket(StrPtrLen* inPacket, void* inStreamCookie, UInt32 inFlags, SInt64 packetLatenessInMSec, SInt64* timeToSendThisPacketAgain, UInt64* packetIDPtr, SInt64* arrivalTimeMSec, Bool16 firstPacket );  

        ReflectorSession* GetReflectorSession();//{ return fReflectorSession; }  
        void        TearDown() {}  
        Bool16      IsUDP() { return true;}  
        Bool16      IsPlaying() { return true;}  

       bool MkFileDirandFile();  
       void Write_Header();  
          
private:  
    
        ReflectorSession*       fReflectorSession;  
        char                    fFilePath[MAXPATHLEN];  
        char                    fFileName[MAXPATHLEN - 20];  
        SInt32                  ffd;  // file handle  
        Bool16  bLastBlockFlg;//  
        SInt64  lastArrivalTimeMSec;           
};

#endif /* RTPSESSIONSAVEOUTPUT_H */

