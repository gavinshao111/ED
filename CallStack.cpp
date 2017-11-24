class RTSPSession : RTSPSessionInterface {
    QTSS_RoleParams fRoleParams;

    SInt64 Run() {

        switch (fState)
            case kReadingFirstRequest
            fInputStream.ReadRequest()

            ...
            case kProcessingRequest:
            (void) theModule->CallDispatch(QTSS_RTSPRequest_Role, &fRoleParams);


    }
}

class RTSPSessionInterface {
    TCPSocket fSocket;
    RTSPRequestStream fInputStream;

    RTSPSessionInterface() {
        fSocket(NULL, Socket::kNonBlockingSocketType),
                fInputStream(&fSocket),
    }

    QTSS_Error InterleavedWrite(void* inBuffer, UInt32 inLen, UInt32* outLenWritten, unsigned char channel) {
        struct RTPInterleaveHeader rih;
        rih.header = '$';
        rih.channel = channel;
        rih.len = htons((UInt16) inLen);

        iov[1].iov_base = (char*) &rih;
        iov[1].iov_len = sizeof (rih);

        iov[2].iov_base = (char*) inBuffer;
        iov[2].iov_len = inLen;

        err = this->GetOutputStream()->WriteV(iov, 3, inLen + sizeof (rih), outLenWritten, RTSPResponseStream::kAllOrNothing);

    }
}

class QTSSModule {
    CallDispatch()
    (fDispatchFunc) (inRole, inParams);
}

QTSSFileModuleDispatch(QTSS_Role inRole, QTSS_RoleParamPtr inParamBlock) {
    switch (inRole)
        case QTSS_RTSPRequest_Role:
        return ProcessRTSPRequest(&inParamBlock->rtspRequestParams);
}

QTSS_Error ProcessRTSPRequest(QTSS_StandardRTSP_Params* inParamBlock) {
    case qtssDescribeMethod:
    err = DoDescribe(inParamBlock);

}

QTSS_Error DoDescribe(QTSS_StandardRTSP_Params* inParamBlock) {
    QTSS_Error err = QTSSModuleUtils::SendErrorResponse(inParamBlock->inRTSPRequest, qtssClientNotFound, sNoSDPFileFoundErr, &pathStr);

}

class RTSPRequestStream {
    TCPSocket* fSocket;

    RTSPRequestStream(TCPSocket* sock)
    : fSocket(sock) {
    }

    ReadRequest() {
        ...
        StrPtrLen* theLocalAddrStr = fSocket->GetLocalAddrStr();
        UInt16 serverPort = fSocket->GetLocalPort(); //%u	
        theLocalAddrStr->PrintStr();


    }

}


EasyDarwin\APIModules\QTSSReflectorModule\QTSSReflectorModule.cpp
Initialize(){
    static OSRefTable* sSessionMap = QTSServerInterface::GetServer()->GetReflectorSessionMap();
}

DoSetup{
    if (!isPush)
        RTPSessionOutput * theNewOutput = new RTPSessionOutput(inParams->inClientSession, theSession, sServerPrefs, sStreamCookieAttr);
    theSession->AddOutput(theNewOutput, true);
    (void) QTSS_SetValue(inParams->inClientSession, sOutputAttr, 0, &theNewOutput, sizeof (theNewOutput));
    else
        StrPtrLen theFileName("test.mp4");
    RTPSessionSaveOutput* theSaveAsMP4 = new RTPSessionSaveOutput(theSession, theFileName.Ptr);
    theSession->AddOutput(theSaveAsMP4, false);}

void RemoveOutput(ReflectorOutput* inOutput, ReflectorSession* inSession, Bool16 killClients) {
    if (inOutput != NULL) {
        // ReflectorSession绉婚櫎��㈡埛绔?        inSession->RemoveOutput(inOutput, true);
    } else
        // 鎺ㄩ€佺�?        SourceInfo* theInfo = inSession->GetSourceInfo();


        delete inOutput;
}

class ReflectorOutput {
}

class RTSPRequestInterface : public QTSSDictionary {\
    RTSPSessionInterface* fSession;

    void WriteStandardHeaders() {
        if (fStatus == qtssClientNotFound) {
            ip = GetSession()->GetSocket()->GetLocalAddrStr()->Ptr;
            port = (int) GetSession()->GetSocket()->GetLocalPort();
            sendStopPushMq();
        }
    }
}

class ReflectorSession {
    unsigned int fNumOutputs;
    OSRef fRef;

    void AddOutput(ReflectorOutput* inOutput, Bool16 isClient) {
        (void) atomic_add(&fNumOutputs, 1);
    }

    void RemoveOutput(ReflectorOutput* inOutput, Bool16 isClient) {
        (void) atomic_sub(&fNumOutputs, 1);
        if (fNumOutputs == 0) {
            //璋冪敤瑙掕壊锛屽仠姝㈡帹�?and send stop MQ

        }
    }


}

class RTPSessionOutput : public ReflectorOutput {
}

class RTPSessionSaveOutput : public ReflectorOutput {
}

class RTPStream {

    QTSS_Error Write(void* inBuffer, UInt32 inLen, UInt32* outLenWritten, UInt32 inFlags) {
        if (inFlags & qtssWriteFlagsIsRTP) {
            if (fTransportType == qtssRTPTransportTypeTCP) // write out in interleave format on the RTSP TCP channel.
                err = this->InterleavedWrite(thePacket->packetData, inLen, outLenWritten, fRTPChannel);

        }
    }

    QTSS_Error InterleavedWrite(void* inBuffer, UInt32 inLen, UInt32* outLenWritten, unsigned char channel) {
        QTSS_Error err = fSession->GetRTSPSession()->InterleavedWrite(inBuffer, inLen, outLenWritten, channel);
        return err;
    }
}

class RTSPResponseStream {

    QTSS_Error RTSPResponseStream::WriteV(iovec* inVec, UInt32 inNumVectors, UInt32 inTotalLength,
            UInt32* outLengthSent, UInt32 inSendType) {
        theErr = fSocket->WriteV(inVec, inNumVectors, &theLengthSent);

    }

}

class Socket {

    OS_Error WriteV(const struct iovec* iov, const UInt32 numIOvecs, UInt32* outLenSent) {
        Assert(iov != NULL);

        if (!(fState & kConnected))
            return (OS_Error) ENOTCONN;

        int err;
        do {
            err = ::writev(fFileDesc, iov, numIOvecs); //flags??
        } while ((err == -1) && (OSThread::GetErrno() == EINTR));

        if (outLenSent != NULL)
            *outLenSent = (UInt32) err;

        return OS_NoErr;
    }
}

QTSServer::Initialize(){
    fReflectorSessionMap = new OSRefTable(kReflectorSessionMapSize/* 5000 */);
}
class OSRefTable{
    OSRefHashTable  fTable;
    OSRefTable(int size):fTable(size){}
}
typedef OSHashTable<OSRef, OSRefKey> OSRefHashTable;
template<class T, class K>
class OSHashTable {
    OSHashTable( UInt32 size )
    {
        fHashTable = new ( T*[size] );
        Assert( fHashTable );
        memset( fHashTable, 0, sizeof(T*) * size );
        fSize = size;
        // Determine whether the hash size is a power of 2
        // if not set the mask to zero, otherwise we can
        // use the mask which is faster for ComputeIndex
        fMask = fSize - 1;
        if ((fMask & fSize) != 0)
            fMask = 0;
        fNumEntries = 0;
    }
    private:
        T** fHashTable;
        UInt32 fSize;
        OSCond  fCond;
}



