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
using namespace std;
static lock_tbl_t lock_tbl;
static lock_tbl_t::iterator it;
static mutex MtxForMap;


shared_ptr<lock_t> get_lock(const string& key) {
    unique_lock<mutex> lk(MtxForMap);
    lock_tbl_t::iterator it;
    if (lock_tbl.end() == it = lock_tbl.find(key)) {
        shared_ptr<lock_t> lock = make_shared<lock_t>();
        return lock_tbl.insert(pair<string, shared_ptr < lock_t >> (key, lock)).second;
    }
    
    return it->second;
}

void unregister(const string& key) {
    unique_lock<mutex> lk(MtxForMap);
    lock_tbl.erase(it);
}

int getSizeofMap() {
    return lock_tbl.size();
}