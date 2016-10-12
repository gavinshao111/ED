/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   GetSession.h
 * Author: 10256
 *
 * Created on 2016年9月22日, 下午8:58
 */

#ifndef GETSESSION_H
#define GETSESSION_H

/*
 * the req is "OPTIONS rtsp://192.168.43.201:8888/record/$1234/1/abc.sdp RTSP/1.0\r\n..."
 * fullFileName is from "record/$1234/1/abc.sdp RTSP/1.0\r\n..."
 * length is strlen("record/$1234/1/abc.sdp");
 */
bool IsUrlExistingInSessionMap(const char *fullFileName, int length);

#endif /* GETSESSION_H */

