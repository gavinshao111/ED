/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   cCondVB.cpp
 * Author: gavin
 * 
 * Created on September 30, 2016, 9:57 AM
 */

#include "cCondVB.h"
#include <stdio.h>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <string.h>
#include <map>

#define TIMEOUT 0
#define WOKENUP 1

using namespace std;

typedef struct sCondVBAndMutexType {
    mutex _mtx;
    condition_variable _cv;
}tCBMType;

typedef map<string, tCBMType*> mapT;

static mapT condVBAndMutexMap;
static mapT::iterator it;
mutex MtxForMap;



static int Register(const string& key)
{
    MtxForMap.lock();
    tCBMType *ct = new tCBMType();
    condVBAndMutexMap.insert(pair<string, tCBMType*>(key, ct));
    MtxForMap.unlock();
    return 0;
}

static int UnRegister(const string& key)
{
    MtxForMap.lock();
    if (condVBAndMutexMap.end() == (it = condVBAndMutexMap.find(key))){
        MtxForMap.unlock();
        return -1;
    }
    delete it->second;
    condVBAndMutexMap.erase(it);
    MtxForMap.unlock();
    return 0;
}

static int getVBMType(const string& key, tCBMType **ppCBMType)
{
    MtxForMap.lock();
    if (condVBAndMutexMap.end() == (it = condVBAndMutexMap.find(key))){
        MtxForMap.unlock();
        return -1;
    }
    *ppCBMType = it->second;        
    MtxForMap.unlock();
    return 0;
}

int waitForPush(const char* fullFileName, int length, int timeout)
{
    if (NULL == fullFileName)
        return -1;
    if (0 == length)
        length = strlen(fullFileName);
    
    string _key(fullFileName, length);
    tCBMType* pCBM = NULL;
    int rc = -1;
    
    // may url is already existing in condVBAndMutexMap.
    Register(_key);
    
    rc = getVBMType(_key, &pCBM);    
    if (0 != rc){
        cerr<<"getVBMType error for "<<_key<<". rc = "<<rc<<endl;
        return -1;
    }
    
    unique_lock<mutex> lk(pCBM->_mtx);
    if (TIMEOUT == pCBM->_cv.wait_for(lk, chrono::seconds(timeout)))
        return -2;

    return 0;
}

int notifyAppThatPushIsArrived(const char* fullFileName, int length)
{
    if (NULL == fullFileName)
        return -1;
    if (0 == length)
        length = strlen(fullFileName);

    string _key(fullFileName, length);
    tCBMType* pCBM = NULL;
    int rc = -1;

    rc = getVBMType(_key, &pCBM);    
    if (0 != rc){
        cerr<<"getVBMType error for "<<_key<<". rc = "<<rc<<endl;
        cerr<<"there is no app waiting for "<<_key<<endl;
        return -1;
    }
    
    unique_lock<mutex> lk(pCBM->_mtx);
    pCBM->_cv.notify_all();    
    
    return 0;
}

int cleanCV(const char* fullFileName, int length)
{
    if (NULL == fullFileName)
        return -1;
    if (0 == length)
        length = strlen(fullFileName);
    
    string _key(fullFileName, length);
    tCBMType* pCBM = NULL;
    int rc = -1;
    
    rc = UnRegister(_key);
    if (0 != rc){
        return -1;
    }
    
    return 0;
}