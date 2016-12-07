/***************************************************************************
 * 
 * Copyright (c) 2014 Sina.com, Inc. All Rights Reserved
 * 
 **************************************************************************/



/**
 * @file bus_util.cc
 * @author liudong2(liudong2@staff.sina.com.cn)
 * @date 2014/04/29 23:32:46
 * @version $Revision$
 * @brief 
 *  
 **/
#include "bus_util.h"
#include <openssl/evp.h>
#include <openssl/aes.h>

namespace bus {

    bool str2num(const char *str, long &num)
    {
        char *endptr;
        num = strtol(str, &endptr, 10);
        if (*str != '\0' && *endptr == '\0')
            return true;
        return false;
    }

    int DecodeBase64Char(unsigned int ch)
    {
        if (ch >= 'A' && ch <= 'Z')
            return ch - 'A' + 0;    // 0 range starts at 'A'
        if (ch >= 'a' && ch <= 'z')
            return ch - 'a' + 26;   // 26 range starts at 'a'
        if (ch >= '0' && ch <= '9')
            return ch - '0' + 52;   // 52 range starts at '0'
        if (ch == '+')
            return 62;
        if (ch == '/')
            return 63;
        return -1;
    }

    int Base64Decode(unsigned char *szSrc, int nSrcLen, unsigned char *pbDest, int *pnDestLen)
    {

        if (szSrc == NULL || pnDestLen == NULL)
        {
            return -1;
        }

        unsigned char *szSrcEnd = szSrc + nSrcLen;
        int nWritten = 0;

        int bOverflow = (pbDest == NULL) ? 1 : 0;
        //printf("over:%d\n", bOverflow);

        while (szSrc < szSrcEnd &&(*szSrc) != 0)
        {       
            unsigned int dwCurr = 0;
            int i;
            int nBits = 0;
            for (i=0; i<4; i++)
            {
                if (szSrc >= szSrcEnd)
                    break;
                int nCh = DecodeBase64Char(*szSrc);
                szSrc++;
                if (nCh == -1)
                {
                    // skip this char
                    i--;
                    continue;
                }
                dwCurr <<= 6;
                dwCurr |= nCh;
                nBits += 6;
            }

            if(!bOverflow && nWritten + (nBits/8) > (*pnDestLen))
                bOverflow = 1;

            //printf("over:%d %d\n", bOverflow, szSrcEnd-szSrc);

            // dwCurr has the 3 bytes to write to the output buffer
            // left to right
            dwCurr <<= 24-nBits;
            for (i=0; i<nBits/8; i++)
            {
                if(!bOverflow)
                {
                    *pbDest = (unsigned char) ((dwCurr & 0x00ff0000) >> 16);
                    pbDest++;
                }
                dwCurr <<= 8;
                nWritten++;
            }
        }

        *pnDestLen = nWritten;

        if(bOverflow)
        {
            if(pbDest != NULL)
            {
                //ATLASSERT(false);
            }

            return -2;
        }

        return 0;
    }

    bool DesEcDncrypt(const unsigned char *pInBuff,unsigned int uInLen,unsigned char *pOutBuff,unsigned int &uCtxLen,const unsigned char* pKey)
    {
        int nc = 0,ct_ptr = 0;
        unsigned int uBlockSize = 0;
        bool bRet = false;

        EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
        if(!ctx)
            return bRet;

        EVP_CIPHER_CTX_init(ctx);
        EVP_DecryptInit(ctx, EVP_des_ecb(),pKey, NULL);
        EVP_CIPHER_key_length(EVP_des_ecb());

        uBlockSize = EVP_CIPHER_CTX_block_size(ctx);
        uCtxLen = uInLen + uBlockSize - (uInLen % uBlockSize);

        unsigned char *cipherBuf = (unsigned char *)new char[uCtxLen+1];
        memset(cipherBuf,0,uCtxLen);

        do
        {
            if(EVP_DecryptUpdate(ctx,cipherBuf, &nc,pInBuff, uInLen) == 0)
                break;

            if(EVP_DecryptFinal(ctx,cipherBuf + nc, &ct_ptr) == 0)
                break;

            uCtxLen = nc+ct_ptr;
            memcpy(pOutBuff,cipherBuf,uCtxLen);
            bRet = true;
        }while(false);

        EVP_CIPHER_CTX_free(ctx);

        if(cipherBuf)
        {
            delete[] cipherBuf;
            cipherBuf = NULL;
        }

        return bRet;
    }

}//namespace bus
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
