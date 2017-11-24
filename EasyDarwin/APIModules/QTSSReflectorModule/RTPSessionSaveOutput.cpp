#include "RTPSessionSaveOutput.h"
#include "fcntl.h"
#include "sys/stat.h"

RTPSessionSaveOutput::RTPSessionSaveOutput(ReflectorSession* inReflectorSession, char* theFileName)
    :fReflectorSession(inReflectorSession),
    ffd(-1)
{
    fprintf(stderr, "------- XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX ,theFileName =%s \n", theFileName);
	::memset(fFileName, 0, sizeof(fFileName));
    ::memset(fFilePath, 0 , sizeof(fFilePath));
    
    bLastBlockFlg = false;
    lastArrivalTimeMSec = -1;
    
    char defaultPath[MAXPATHLEN] = {0};
    snprintf(defaultPath, MAXPATHLEN - 1, "%s/EasyDarwin/Movies/pushStream/", getenv("ED"));
    strcpy(fFilePath, defaultPath);
    char *firstPos = ::strstr(theFileName, ".sdp");
    if (firstPos != NULL){
        int length = strlen(theFileName);
        int i = length - 1;
        int end = i;
        while(i > 0){
            if (theFileName[i] == firstPos[0]){
                end = i;
            }

            if (theFileName[i] == '/'){
                fprintf(stderr, "----setup path:%d, i:%d-------\n", end, i);
                break;
            }

            i--;
        }
        if (i > 0 && end > i + 1){
            memcpy((char *)fFileName, ((char*)theFileName + i + 1), end - i - 1);
            fFileName[end - i - 1] = '\0';
            snprintf(fFilePath, MAXPATHLEN - 1, "%s%s", defaultPath, fFileName);
            fprintf(stderr, "----fFileName:%s, path:%s-------\n", fFileName, fFilePath);
        }
    }
    
    MkFileDirandFile();

    fprintf(stderr, "-------  RTPSessionSaveAsMP4,  GetNumStreams=%d \n", inReflectorSession->GetNumStreams());
    this->InititializeBookmarks( inReflectorSession->GetNumStreams());

}


RTPSessionSaveOutput::~RTPSessionSaveOutput()
{
    fprintf(stderr, "------- YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY \n");
    if(ffd != -1)
    {
        close(ffd);
        ffd = -1;
    }
	::memset(fFileName, 0, sizeof(fFileName));
    ::memset(fFilePath, 0 , sizeof(fFilePath));
}

bool RTPSessionSaveOutput::MkFileDirandFile()
{        
    if(access(fFilePath, R_OK) != 0) // dir exists
    {
      int isCreate = mkdir(fFilePath , S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
      if( !isCreate )    
        fprintf(stderr, "create dir ok, %s\n", fFileName);
      else
      {
        fprintf(stderr, "create %s failed! error code : %d\n", fFilePath, isCreate);
        return false;
      }
    }

   fprintf(stderr, "---------- dir is ok ---------------%s\n", fFileName);    

    time_t	tmt =  OS::UnixTime_Secs();	 // Seconds since 1970  				
    char szBuf[128] = {0};        
    strftime(szBuf, sizeof(szBuf), "%Y-%m-%d_%H_%M_%S", localtime(&tmt));
    fprintf(stderr, " start0 =  %s\n", szBuf);

    if(-1 == ffd)
    {
        sprintf(fFilePath, "%s/%s.m4a", fFilePath, szBuf);
        fprintf(stderr, "-------open file , fFilePath=%s  \n", fFilePath);
        ffd = open(fFilePath, O_CREAT  | O_WRONLY  | O_APPEND, 0666);

        if (ffd != -1){
            //Write_Header();
        }
    }

    return true;
}
    
ReflectorSession* RTPSessionSaveOutput::GetReflectorSession() 
{
    fprintf(stderr, "################################## RTPSessionSaveAsMP4::GetReflectorSession \n");
    return fReflectorSession; 
}

void RTPSessionSaveOutput::Write_Header()
{
    if (1){
        unsigned char header[4]; //  H264 header
        header[0] = 0;
        header[1] = 0;
        header[2] = 0;
        header[3] = 1;

        write(ffd, header, 4);
    }else{
        unsigned char header[6] = "\0"; //  AMR header
        header[0] = 0x23;
        header[1] = 0x21;
        header[2] = 0x41;
        header[3] = 0x4D;
        header[4] = 0x52;
        header[5] = 0x0A;
        
        write(ffd, header, sizeof(header));
    }
}

//这个方法主要实现了AAC RTP音频流的存储，将存储的文件拷贝出来后，能正常播放
QTSS_Error  RTPSessionSaveOutput::WritePacket(StrPtrLen* inPacket, void* inStreamCookie, UInt32 inFlags, SInt64 packetLatenessInMSec, SInt64* timeToSendThisPacketAgain, UInt64* packetIDPtr, SInt64* arrivalTimeMSecPtr, Bool16 firstPacket)
{
    fprintf(stderr, "[DEBUG] RTPSessionSaveOutput::WritePacket\n\n");
    if (inPacket == NULL || inPacket->Len < 14){
         
                fprintf(stderr, "inPacket == NULL || inPacket->Len < 14");
	  return QTSS_NoErr;
    }
    
    SInt64                  currentTime = OS::Milliseconds();

   if(-1 != ffd)
   {
        UInt8* payload = (UInt8*)inPacket->Ptr;
        int start = 12; //rtp header lengthavformat_write_header
	//just save aac audio.
	if (inPacket->Len < 16){
		return QTSS_NoErr;
	}
	if (payload[start] != 0x00 || payload[start + 1] != 0x10){
		return QTSS_NoErr;
	}
	//check end.
	
        int rtptype = payload[start] & 0x1f;
        //int payload_type = payload[1]& 0x7f;
        bool bStartFlg = ((payload[start+1] & 0x80) == 0x80) ? true : false;
        
        //fprintf("#################   rtptype  = %d \n", rtptype);
#if 1//这里用来做H264文件的存储
        if (rtptype== 28) { // FU-A

            //fprintf("################# rtptype== 28, arrivalTimeMSecPtr=%ld, len =%d\n", *arrivalTimeMSecPtr, inPacket->Len - start -2);
            if(bStartFlg) 
            {
                Write_Header();
            }
            
            int ret = write(ffd, (void*)(payload + start + 2), (inPacket->Len - start -2));
            if(ret == -1)
            {
                fprintf(stderr, "error=%d, strerror=%s", errno, strerror(errno));
            }               
        } else if (rtptype <= 24){
            // single nal unit
            // fprintf("####################### rtptype rtptype =%d, arrivalTimeMSecPtr=%ld, len =%d\n", rtptype ,*arrivalTimeMSecPtr,  (inPacket->Len - start));

            //Write_Header();
            int ret = write(ffd, (void*)(payload+start), (inPacket->Len - start));
            // 00 00 00 01 
            if(ret == -1)
            {
                fprintf(stderr, "error=%d, strerror=%s", errno, strerror(errno));
            }
        }
#else
        if (lastArrivalTimeMSec >= *arrivalTimeMSecPtr){
            return QTSS_NoErr;
        }
        fprintf(stderr, "RTPSessionSaveAsMP4::WritePacket lastArrivalTimeMSec=%d\n", lastArrivalTimeMSec);
        lastArrivalTimeMSec = *arrivalTimeMSecPtr;

    #if 1        
            //aac header
            unsigned char adtsHeader[7] = {0};


            adtsHeader[0] = 0xFF;
            adtsHeader[1] = 0xF1;
            int profile = 2;
            int freqIdx = 11;
            int chanCfg = 1; 
            int packetLen = inPacket->Len - start + 7 - 4;

            adtsHeader[2] = ((profile -1 )<<6) + (freqIdx << 2) + (chanCfg >> 2);
            adtsHeader[3] = ((chanCfg & 3) << 6) + (packetLen >> 11);
            adtsHeader[4] = (packetLen & 0x7ff) >> 3;//(packetLen >> 3) & 0xff;
            adtsHeader[5] = ((packetLen & 0x7) << 5)|0x1f;
            adtsHeader[6] = 0xFC;
            int ret = write(ffd, adtsHeader, sizeof(adtsHeader));
    #endif//     
        //amr header
        //aaclatm 4header
        
        ret = write(ffd, (void*)(payload + start + 4), (inPacket->Len - start - 4));
        if(ret == -1)
        {
            fprintf(stderr, "error=%d, strerror=%s", errno, strerror(errno));
        }
        fLastIntervalMilliSec = currentTime - fLastPacketTransmitTime;
        if (fLastIntervalMilliSec > 100) {
        //reset interval maybe first packet or it has been blocked for awhile
        fLastIntervalMilliSec = 5;
        }

        fLastPacketTransmitTime = currentTime;
        if (!bLastBlockFlg){
        bLastBlockFlg = true;
        }else{
        bLastBlockFlg = false;
        }
#endif

        //flush(ffd);
   }
    
  return QTSS_NoErr;
}