#include <stdarg.h>
#include <string.h>
#include "myredis.h"

namespace bus
{
    extern bus_log_t    g_logger;

    MyRedis::MyRedis()
        :m_Reply(NULL)
         ,m_InnerReply(NULL)
         ,m_Redis(NULL)
    {
         m_nAppendCmdCount = 0;
    }
    MyRedis::~MyRedis()
    {
        if (m_Redis)
        {
            delete m_Redis;
            m_Redis = NULL;
        }
        if (m_Reply)
        {
            delete m_Reply;
            m_Reply = NULL;
        }
        if (m_InnerReply)
        {
            delete m_InnerReply;
            m_InnerReply = NULL;
        }
    }
    int MyRedis::Init(const char* ip, int port, bool bRedisNeedPassword, const char *szRedisPassword)
    {
        int ret = -1;
        if (0 == strcmp(ip,"") || 0 == port)
        {
            g_logger.error("传入参数非法： ip :%s port :%d", m_szIp, m_hdPort);
            return ret;
        }

        memset (m_szIp, 0, sizeof(m_szIp));
        memset(m_szRedisPassword, 0, sizeof(m_szRedisPassword));
        strncpy(m_szIp, ip, sizeof(m_szIp));
        m_hdPort = port;

        m_bRedisNeedPassword = bRedisNeedPassword;
        strncpy(m_szRedisPassword, szRedisPassword, sizeof(m_szRedisPassword));

        m_Redis = redisConnect(m_szIp, m_hdPort); 
        if(NULL == m_Redis || m_Redis->err)  
        {    
            if(m_Redis)
            {    
                redisFree(m_Redis); 
                m_Redis = NULL;            
            }    

            g_logger.error("Connect to redisServer<%s:%hd> fail",m_szIp, m_hdPort);  
            return ret; 
        } 

        ret = Auth();

        return ret;
    }

    int MyRedis::Auth()
    {
        if (m_bRedisNeedPassword)
        {
            m_InnerReply = (redisReply *) redisCommand(m_Redis, "AUTH %s", m_szRedisPassword);
            if (m_InnerReply->type == REDIS_REPLY_ERROR)
            {
                g_logger.error("auth redisServer<%s:%hd> fail, password:%s",m_szIp, m_hdPort, m_szRedisPassword);  
                return -1;
            }

            g_logger.notice("auth redisServer<%s:%hd> succeed",m_szIp, m_hdPort);  
            if (m_InnerReply)
            {
                freeReplyObject(m_InnerReply);
                m_InnerReply = NULL;
            }
        }
        return 0;
    }

    //set 命令  不需要返回值  不释放上次的m_Reply
    int MyRedis::RedisCmdNoReply(const char* pszFormat, ...)
    {
        va_list argList;
        int ret = -1;
        bool bOK =false;
        bool bReadOnlyError = false;
        bool bNoScriptError = false;
        bool bTryAgain = false;
        int i = 0;

        //连接成功 执行redis 命令
        do
        {
            if (bTryAgain)
            {
                if(m_Redis)
                {
                    redisFree(m_Redis);
                    m_Redis = NULL;
                }
                if(m_InnerReply)
                {
                    freeReplyObject(m_InnerReply);
                    m_InnerReply = NULL;
                }       
            }       
            if(NULL == m_Redis )
            {
                m_Redis = redisConnect(m_szIp, m_hdPort);
                if(NULL == m_Redis || m_Redis->err)  
                {     
                    g_logger.error("Connect to redisServer<%s:%hd> fail, try:%d",m_szIp, m_hdPort, i);  
                    bTryAgain = true;
                    i++;
                    usleep(500);
                    continue;  
                }  
                ret = Auth();
                if (ret != 0)
                {
                    bTryAgain = true;
                    i++;
                    continue;
                }
            }
            va_start( argList, pszFormat );
            m_InnerReply = (redisReply*)redisvCommand(m_Redis, pszFormat, argList);
            va_end( argList );
            if(NULL != m_InnerReply) 
            { 
                bOK = false;
                //ret = 1;
                switch(m_InnerReply->type) 
                {
                    case REDIS_REPLY_STATUS:
                        if(!strcasecmp(m_InnerReply->str,"OK"))
                            bOK = true;
                        break;

                    case REDIS_REPLY_ERROR:
                        break;

                    default:
                        bOK = true;
                        break;
                }

                if(!bOK) 
                { 
                    if(REDIS_REPLY_ERROR == m_InnerReply->type && strstr(m_InnerReply->str,"READONLY You can't write against a read only slave"))
                    {
                        g_logger.error("redis read only error!");
                        bReadOnlyError = true;
                    }
                    else if(REDIS_REPLY_ERROR == m_InnerReply->type && strstr(m_InnerReply->str,"NOSCRIPT No matching script. Please use EVAL"))
                    {
                        g_logger.error(" %s script not exists error!", __FUNCTION__);
                        bNoScriptError = true;
                    }
                    g_logger.error("redisCommand exec fail,%d %s %lld:",m_InnerReply->type,m_InnerReply->str,m_InnerReply->integer);
                    if(bReadOnlyError)
                    {
                        if(m_Redis)
                        {
                            redisFree(m_Redis); 
                            m_Redis = NULL;
                        }
                        g_logger.error("redis read only error,ret :%d try:%d",  ret, i);
                        ret = -2;
                        bTryAgain = true;
                        i++;
                        continue;
                    }
                    else if(bNoScriptError)
                    {
                        ret = -3;
                    }
                    else
                    {
                        ret = -4;
                    }
                }
                ret = 0;
                bTryAgain =false;
            }
            else 
            {  
                if(m_Redis)
                {
                    redisFree(m_Redis); 
                    m_Redis = NULL;
                }
                g_logger.error("redisCommand fail, m_InnerReply is NULL, try：%d", i);
                bTryAgain = true;
                i++;
                continue;
            }
        }while(bTryAgain && i < 3);    

        if (m_InnerReply)
        {
            freeReplyObject(m_InnerReply);
            m_InnerReply = NULL;
        }

        return ret;
    }
    //get 命令 需要返回值  释放上次的m_Reply
    //在执行get命令前 要先获取到上次命令的结果 
    int MyRedis::RedisCmdWithReply(const char* pszFormat, ...)
    {
        va_list argList;
        int ret = -1;
        bool bOK =false;
        bool bReadOnlyError = false;
        bool bNoScriptError = false;
        bool bTryAgain = false;
        int i = 0;

        if (m_Reply)
        {
            freeReplyObject(m_Reply);
            m_Reply = NULL;
        }

        //连接成功 执行redis 命令
        do
        {

            if(bTryAgain)
            {
                if(m_Redis)
                {
                    redisFree(m_Redis);
                    m_Redis = NULL;
                }
                if(m_Reply)
                {
                    freeReplyObject(m_Reply);
                    m_Reply = NULL;
                }       
            }       
            if(NULL == m_Redis )
            {
                m_Redis = redisConnect(m_szIp, m_hdPort);
                if(NULL == m_Redis || m_Redis->err)  
                {     
                    g_logger.error("Connect to redisServer<%s:%hd> fail, try:%d",m_szIp, m_hdPort, i);  
                    bTryAgain = true;
                    i++;
                    usleep(500);
                    continue;  
                }  
                ret = Auth();
                if (ret != 0)
                {
                    bTryAgain = true;
                    i++;
                    continue;
                }
            }

            va_start( argList, pszFormat );
            m_Reply = (redisReply*)redisvCommand(m_Redis, pszFormat, argList);
            va_end( argList );
            if(NULL != m_Reply) 
            { 
                bOK = false;
                switch(m_Reply->type) 
                {
                    case REDIS_REPLY_STATUS:
                        if(!strcasecmp(m_Reply->str,"OK"))
                            bOK = true;
                        break;

                    case REDIS_REPLY_ERROR:
                        break;

                    case REDIS_REPLY_NIL:
                        break;
                    default:
                        bOK = true;
                        break;
                }

                if(!bOK) 
                { 
                    if (REDIS_REPLY_NIL == m_Reply->type)
                    {
                        g_logger.error("key is not exist, get failed, reply type :%d", m_Reply->type);
                        return -1;
                    }
                    if(REDIS_REPLY_ERROR == m_Reply->type && strstr(m_Reply->str,"READONLY You can't write against a read only slave"))
                    {
                        g_logger.error("redis read only error!");
                        bReadOnlyError = true;
                    }
                    else if(REDIS_REPLY_ERROR == m_Reply->type && strstr(m_Reply->str,"NOSCRIPT No matching script. Please use EVAL"))
                    {
                        g_logger.error("script not exists error!");
                        bNoScriptError = true;                                                                                                       
                    }
                    g_logger.error("redisCommand exec fail,%d %s %lld:",m_Reply->type,m_Reply->str,m_Reply->integer);
                    if(bReadOnlyError)
                    {
                        if(m_Redis)
                        {
                            redisFree(m_Redis); 
                            m_Redis = NULL;
                        }

                        ret = -2;
                        bTryAgain = true;
                        i++;
                        continue;
                    }
                    else if(bNoScriptError)
                    {
                        ret = -3;
                        break;
                    }
                    else
                    {
                        ret = -4;
                        break;
                    }
                }
                ret = 0;
                bTryAgain =false;
            }
            else 
            {  
                g_logger.error("redisCommand fail ");
                if(m_Redis)
                {
                    redisFree(m_Redis); 
                    m_Redis = NULL;
                }
                bTryAgain = true;
                i++;
                continue;
            }
        }while(bTryAgain && i < 3);    
        return ret;
    }

    int MyRedis::RedisPipeAppendCmd(const char *szFormat, ...)
    {    
        int nRet = 0;

        va_list arglist;
        va_start(arglist, szFormat);
        nRet = redisvAppendCommand(m_Redis, szFormat, arglist);
        va_end(arglist);
        if (nRet != REDIS_OK)
        {
            g_logger.error("RedisPipeAppendCmd error, maybe outoff memory.", m_nAppendCmdCount);
            return nRet;
        }

        m_nAppendCmdCount++;
        return nRet;
    }


    int MyRedis::RedisPipeGetResult()
    {
        redisReply *reply;
        int nRet = 0;

        for ( ; m_nAppendCmdCount > 0; m_nAppendCmdCount--)
        {
            nRet = redisGetReply(m_Redis, (void **)&reply);
            for (int i = 0; nRet != 0 && i < 3; i++)
            {
                Reconnect();
                usleep(500);
                nRet = redisGetReply(m_Redis, (void **)&reply);
                if (nRet == 0)
                    break;
            }

            if (nRet != 0)
            {
                g_logger.error("RedisPipeGetResult error. m_nAppendCmdCount=%d", m_nAppendCmdCount);
                return nRet;
            }
        }

        return 0;
    }

    int MyRedis::Reconnect()
    {
        if(m_Redis)
        {
            redisFree(m_Redis);
            m_Redis = NULL;
        }

        m_Redis = redisConnect(m_szIp, m_hdPort);
        if(NULL == m_Redis || m_Redis->err)  
        {     
            g_logger.error("Connect to redisServer<%s:%hd> fail",m_szIp, m_hdPort);  
            return -1;
        }  
        return Auth();
    }

    int MyRedis::RedisCmd(const char* szFormat, ...)
    {
        va_start( argList, szFormat );
        m_Reply = (redisReply*)redisvCommand(m_Redis, szFormat, argList);
        va_end( argList );
        for (int i = 0; m_Reply == NULL && i < 3; i++)
        {
            Reconnect();
            usleep(500);
            m_Reply = (redisReply *)redisvCommand(m_Redis, szFormat, argList);
            if (m_Reply)
                break;
        }

        if (m_Reply->type == REDIS_REPLY_ERROR)
        {
            g_logger.error("exec cmd error. errstr:%s", m_Reply->str);  
            return -1;
        }
        else if (m_Reply->type == REDIS_REPLY_STATUS)
        {
            if (strcasecmp(m_Reply->str, "OK") == 0)
            {
                return 0;
            }
        }
        else if (m_Reply->type == REDIS_REPLY_NIL)
        {
            g_logger.error("get nil result");
            return 0;
        }

        return 0;
    }
}
