/***************************************************************************
 * 
 * Copyright (c) 2014 Sina.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
/**
 * @file bus_log.h
 * @author liudong2(liudong2@staff.sina.com.cn)
 * @date 2014/03/09 09:30:18
 * @version $Revision$
 * @brief 定义日志模块
 *  
 **/
#ifndef  __BUS_UTIL_H_
#define  __BUS_UTIL_H_
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<stdint.h>
#include<stdarg.h>
#include<time.h>
#include<string.h>
#include<pthread.h>
#include<arpa/inet.h>
#include<ifaddrs.h>
#include "bus_log.h"

namespace bus{
bool str2num(const char *str, long &num);

int DecodeBase64Char(unsigned int ch);

int Base64Decode(unsigned char *szSrc, int nSrcLen, unsigned char *pbDest, int *pnDestLen);

bool DesEcDncrypt(const unsigned char *pInBuff,unsigned int uInLen,unsigned char *pOutBuff,unsigned int &uCtxLen,const unsigned char* pKey);

}//namespace bus
#endif  //__BUS_UTIL_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
