#include "business.h"


int Business::Init()
{
    int nRet = 0;
    m_pRedis = new MyRedis();
    if (m_pRedis == NULL)
    {   
        DOLOG("[error]%s new MyRedis error", __FUNCTION__);
        return -1; 
    }   

    string sRedisIp = g_pConfig->GetConfigStr("redis", "redis_ip", "");
    int nRedisPort = g_pConfig->GetConfigInt("redis", "redis_port", 0);
    int nNeedPasswd = g_pConfig->GetConfigInt("redis", "needpasswd", 0);
    string sRedisPasswd = g_pConfig->GetConfigStr("redis", "password", "");

    bool bNeedPasswd = (nNeedPasswd == 0) ? false : true;

    nRet = m_pRedis->Init(sRedisIp.c_str(), nRedisPort, bNeedPasswd, sRedisPasswd.c_str());
    if (nRet != 0)
        return nRet;

    return 0;
}

int Business::IncrProcess(Row *pRow)
{
    DOLOG("[trace]%s", __FUNCTION__);
    pRow->Print();
    return 0;
}

int Business::SaveNextReqPos(string& sBinlogFileName, uint32_t uBinlogPos)
{
    if (m_pRedis == NULL)
        return -1;

    if (sBinlogFileName.size() == 0 || uBinlogPos == 0)
        return 0;

    DOLOG("[trace]%s, filename:%s, logpos:%u", __FUNCTION__, sBinlogFileName.c_str(), uBinlogPos);
    /*
     *@todo 重连后请求的位置，存入redis
     */
    m_pRedis->RedisCmdNoReply("hmset binlogsub_pos_hash filename %s pos %d", sBinlogFileName.c_str(), uBinlogPos);
    return 0;
}

int Business::ReadNextReqPos(string& sBinlogFileName, uint32_t &uBinlogPos)
{
    if (m_pRedis == NULL)
        return -1;

    int nRet = m_pRedis->RedisCmdWithReply("hmget binlogsub_pos_hash filename pos");
    if (nRet < 0)
    {
        DOLOG("[error]%s fail to exec hmget binlogsub_pos_hash filename pos", __FUNCTION__);
        return -1;
    }
    redisReply *pReply = m_pRedis->GetReply();
    if (REDIS_REPLY_NIL == pReply->element[0]->type || REDIS_REPLY_NIL == pReply->element[1]->type)
    {
        DOLOG("[error]%s fail to get logpos from redis.not exists key", __FUNCTION__);
        return -1;
    }

    sBinlogFileName.assign(pReply->element[0]->str);
    uBinlogPos = atol(pReply->element[1]->str);
    DOLOG("[trace]%s filename:%s, logpos:%u", __FUNCTION__, sBinlogFileName.c_str(), uBinlogPos);
    return 0;
}
