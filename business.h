#ifndef BUSINESS_H
#define BUSINESS_H
#include "row.h"
#include "log.h"
#include "myredis.h"
#include "config.h"

class IBusiness
{
    public:
        virtual int IncrProcess(Row *pRow) = 0;
        virtual int SaveNextReqPos(string& sBinlogFileName, uint32_t uBinlogPos) = 0;
        virtual int ReadNextReqPos(string& sbinlogFileName, uint32_t &BinlogPos) = 0;
};

class Business : public IBusiness
{
    public:
        Business() : m_pRedis(NULL) {};
        ~Business(){};

        int Init();

        virtual int IncrProcess(Row *pRow);
        virtual int SaveNextReqPos(string& sBinlogFileName, uint32_t uBinlogPos);
        virtual int ReadNextReqPos(string& sBinlogFileName, uint32_t &uBinlogPos);

    private:
        MyRedis     *m_pRedis;

};
#endif
