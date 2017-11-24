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

#include <iostream>
#include <mutex>                // std::mutex, std::unique_lock
#include <condition_variable>    // std::condition_variable
#include <string>
#include <map>

using std::mutex;
using std::condition_variable;
using std::map;
using std::string;


typedef map<string, std::shared_ptr<lock_t>> lock_tbl_t;



std::shared_ptr<lock_t> get_lock(const string& key);
void unregister(const string& key);
int waitForCond(const string& key);
int getVBMType(const string& key, lock_t **ppCBMType);


};

int getSizeofMap();

#endif /* CCONDVB_H */

