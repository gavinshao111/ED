/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   cCondVB.h
 * Author: gavin
 *
 * Created on September 30, 2016, 9:57 AM
 */

#ifndef CCONDVB_H
#define CCONDVB_H


/*
 * when ED receive app option req, and the url is not existing in map,
 * then call this func to wait.
 * 
 * if req is like "OPTIONS rtsp://192.168.43.201:8888/record/$1234/1/abc.sdp RTSP/1.0\r\n..."
 * fullFileName is like "record/$1234/1/abc.sdp RTSP/1.0\r\n..."
 * length is strlen("record/$1234/1/abc.sdp");
 * 
 * rc:
 *  0   ok
 *  -1  error
 *  -2  timeout
 */
int waitForPush(const char* fullFileName, int length, int timeout);

/*
 * when ED receive Car option, then notify app to wake up from wait.
 * 
 * 
 */
int notifyAppThatPushIsArrived(const char* fullFileName, int length);

/*
 * rc:
 *  0 ok
 *  -1 fullFileName is not existing in CV map.
 */
int cleanCV(const char* fullFileName, int length);

#endif /* CCONDVB_H */

