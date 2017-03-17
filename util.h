#ifndef UTIL_H
#define UTIL_H

#include <string.h>
#include <stdint.h>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include "myconvert.h"

bool str2num(const char *str, long &num);

int DecodeBase64Char(unsigned int ch);

int Base64Decode(unsigned char *szSrc, int nSrcLen, unsigned char *pbDest, int *pnDestLen);

bool DesEcDncrypt(const unsigned char *pInBuff,unsigned int uInLen,unsigned char *pOutBuff,unsigned int &uCtxLen,const unsigned char* pKey);

/*解析压缩过的int类型，lenenc-int*/
bool UnpackLenencInt(char *pBuf, uint32_t &uPos, uint64_t &uValue);

#endif
