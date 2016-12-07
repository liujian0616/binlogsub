#ifndef __MYREDIS_H_20160509__
#define __MYREDIS_H_20160509__

#include <hiredis/hiredis.h>
#include <string>
#include "bus/bus_log.h"

namespace  bus
{
class MyRedis
{
    public:

        MyRedis();
        ~MyRedis();

        //建立redis连接 成功返回0  失败返回-1
        int Init(const char* ip, int port, bool bRedisNeedPassword, const char *szRedisPassword);

        //执行redis命令 根据返回值m_Reply 判断是否成功 成功返回0 失败返回-1(reply为空 或redis连接失败) -2(只读) -3(脚本加载错误) -4(其他)
        //set 命令  不需要返回值  不释放上次的m_Reply
        int RedisCmdNoReply(const char* pszFormat, ...);
        //get 命令  需要返回值  释放上次的m_Reply
        int RedisCmdWithReply(const char* pszFormat, ...);

        int RedisCmd(const char* szFormat, ...);
        int RedisPipeAppendCmd(const char *szFormat, ...);
        int RedisPipeGetResult();
        int Reconnect();
        

        int Auth();


        inline int GetReplyInt(int &reply)const {return (m_Reply ? (reply = m_Reply->integer, 0) : -1);}

        inline char* GetReplyStr() const {return (m_Reply ? m_Reply->str : NULL);}

        inline redisReply* GetReply() const {return m_Reply;};

    private:
        redisReply*     m_Reply;
        redisReply*     m_InnerReply;
        redisContext*   m_Redis;
        unsigned short  m_hdPort;
        char            m_szIp[128];
        char            m_szRedisPassword[128];
        bool            m_bRedisNeedPassword;
        va_list         argList;

        int             m_nAppendCmdCount;
};
}


#endif
