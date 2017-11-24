class EventThread{
	void Start(){
		int err = pthread_create((pthread_t*)&fThreadID, theAttrP, _Entry, (void*)this);				
	}
	
	void* _Entry(void *inThread){
		theThread->fThreadID = (pthread_t)pthread_self();
		pthread_setspecific(OSThread::gMainKey, theThread);
		theThread->Entry();
		
	}
	void Entry(){
		int theReturnValue = epoll_waitevent(&theCurrentEvent, NULL);
		theContext->ProcessEvent(theCurrentEvent.er_eventbits);
	}
}

class OSThread{
	virtual void Entry() = 0;
	
	void Start(){
		pthread_attr_t* theAttrP;
		pthread_create((pthread_t*)&fThreadID, theAttrP, _Entry, (void*)this);
		
	}
	void* _Entry(void *inThread){
		OSThread* theThread = (OSThread*)inThread;
		theThread->fThreadID = (pthread_t)pthread_self();
		pthread_setspecific(OSThread::gMainKey, theThread);
		theThread->Entry();
	}

}
class TaskThread : public OSThread{
	
	virtual void Entry(){
		Task* theTask = NULL;
		theTask = this->WaitForTask();
		
		theTimeout = theTask->Run();
	}
	
}
class Task{
	
	virtual SInt64 Run() = 0;
}

class RTSPSessionInterface : public QTSSDictionary, public Task{}

class RTSPSession : public RTSPSessionInterface{
	SInt64 Run(){
		...
	}
}


(char *)theTask->fTaskName