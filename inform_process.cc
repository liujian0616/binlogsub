#include "inform_process.h"

namespace bus
{

    extern bus_log_t    g_logger;
    //extern bus_config_t g_config;


    bus_inform_process::bus_inform_process()
    {
        m_pRedis = NULL;
        m_pBuf = NULL;
    }

    bus_inform_process::~bus_inform_process()
    {
        if (m_pRedis != NULL)
        {
            delete m_pRedis;
            m_pRedis = NULL;
        }
        if (m_pBuf != NULL)
        {
            delete m_pBuf;
            m_pBuf = NULL;
        }

        DisConnectMysql();
    }

    int bus_inform_process::Init(bus_config_t *pConfig)
    {
        int nRet = 0;
        nRet = Init(pConfig->_mysql_ip, pConfig->_mysql_port, pConfig->_mysql_username, pConfig->_mysql_userpasswd, pConfig->_password_need_decode, pConfig->_redis_ip, pConfig->_redis_port, pConfig->_redis_need_passwd, pConfig->_redis_passwd);
        return nRet;
    }

    int bus_inform_process::Init(const char *szMysqlIp, int nMysqlPort, const char *szMysqlUserName, const char *szMysqlPasswd, bool bPasswdNeedDecode, const char *szRedisIp, int nRedisPort, bool bRedisNeedPasswd, const char *szRedisPasswd)
    {
        int nRet = 0;

        m_pBuf = new char[1024 * 100];
        if (m_pBuf == NULL)
        {
            g_logger.error("new m_pBuf error");
            return -1;
        }


        m_pRedis = new MyRedis();
        if (m_pRedis == NULL)
        {
            g_logger.error("new MyRedis error");
            return -1;
        }

        nRet = m_pRedis->Init(szRedisIp, nRedisPort, bRedisNeedPasswd, szRedisPasswd);
        if (nRet != 0)
            return nRet;

        struct hostent *pHostEnt = gethostbyname(szMysqlIp);
        struct in_addr addr;
        addr.s_addr = *(unsigned long *)pHostEnt->h_addr_list[0];
        char *szRealIp = inet_ntoa(addr);
        strncpy(m_szMysqlIp, szRealIp, sizeof(m_szMysqlIp));

        m_nMysqlPort = nMysqlPort;
        strncpy(m_szMysqlUserName, szMysqlUserName, sizeof(m_szMysqlUserName));

        //解密password
        if (bPasswdNeedDecode)
        {
            int iLen = 64;
            unsigned char pSwap[64] = {0};
            unsigned char szPassword[512] = {0};
            Base64Decode((unsigned char *)szMysqlPasswd, strlen(szMysqlPasswd), pSwap, &iLen);
            u_int pOutlen = 0;
            if (!DesEcDncrypt(pSwap, iLen, szPassword, pOutlen, (unsigned char *)"WorkECJol"))
            {
                g_logger.error("Get mysql password error %s", szMysqlPasswd);
                return false;
            }
            strncpy(m_szMysqlPasswd, (const char *)szPassword, sizeof(m_szMysqlPasswd));
        }
        else
        {
            strncpy(m_szMysqlPasswd, szMysqlPasswd, sizeof(m_szMysqlPasswd));
        }

        nRet = ConnectMysql();
        if (nRet == -1)
        {
            g_logger.error("connect mysql error");
            return -1;
        }
        return nRet;
    }

    int bus_inform_process::FullPull()
    {
        g_logger.notice("%s", __FUNCTION__);
        FullPullFriended();
        FullPullStaff();
        FullPullOnlineCs();
        FullPullOnlineCsLeader();
        return 0;
    }

    int bus_inform_process::IncrProcess(row_t *row)
    {
        //printf("action:%d, database:%s, table:%s, column_count:%d\n", row->get_action(), row->get_db_name(), row->get_table(), row->size());
        g_logger.notice("action:%d, database:%s, table:%s, column_count:%d", row->get_action(), row->get_db_name(), row->get_table(), row->size());

        if (strcmp(row->get_db_name(), "d_ec_user") == 0 && strcmp(row->get_table(), "t_user_group") == 0)
        {
            IncrProcessFriendedAndStaff(row);
        }
        else if (strcmp(row->get_db_name(), "d_ec_cs") == 0 && strcmp(row->get_table(), "t_online_cs") == 0)
        {
            IncrProcessOnlineCs(row);
        }
        else if (strcmp(row->get_db_name(), "d_ec_role") == 0 && strcmp(row->get_table(), "t_account_role") == 0)
        {
            IncrProcessCsLeader(row);
        }

        return 0;
    }

    int bus_inform_process::SaveNextreqPos(const char *szBinlogFileName, uint32_t uBinlogPos)
    {

        if (m_pRedis == NULL)
            return -1;

        if (szBinlogFileName[0] == 0 || uBinlogPos == 0)
            return 0;

        g_logger.debug("save nextreq  logpos in redis, filename:%s, logpos:%u", szBinlogFileName, uBinlogPos);
        /*
         *@todo 重连后请求的位置，存入redis
         */
        m_pRedis->RedisCmdNoReply("hmset informhubsvr_binlog_pos_hash filename %s pos %d", szBinlogFileName, uBinlogPos);
        return 0;
    }

    int bus_inform_process::ReadNextreqPos(char *szBinlogFileName, uint32_t uFileNameLen, uint32_t *puBinlogPos)
    {
        if(m_pRedis == NULL)
        {
            return -1;
        }

        int ret = m_pRedis->RedisCmdWithReply("hmget informhubsvr_binlog_pos_hash filename pos");
        if (0 > ret)
        {
            g_logger.error("fail to exec hmget informhubsvr_binlog_pos_hash filename pos ");
            return -1;
        }
        redisReply *pReply = m_pRedis->GetReply();
        if (REDIS_REPLY_NIL == pReply->element[0]->type || REDIS_REPLY_NIL == pReply->element[1]->type)
        {
            g_logger.error("fail to get logpos from redis.not exists key");
            return -1;

        }

        strncpy(szBinlogFileName,pReply->element[0]->str, uFileNameLen);
        *puBinlogPos = atol(pReply->element[1]->str);
        g_logger.debug("read nextreq  logpos from redis. filename:%s, logpos:%u", szBinlogFileName, *puBinlogPos);
        return 0;
    }

    /*
     *@todo 全量拉取好友关系
     */
    int bus_inform_process::FullPullFriended()
    {
        int nRet = 0;
        uint32_t uLimitOffset = 0;
        uint32_t uLimitCount = 5000;
        int nUserId = 0;
        int nGroupId = 0;
        int nType = 0;
        MYSQL_RES *pRes = NULL;
        MYSQL_ROW row = NULL;
        int nCurRowCount = 0;
        int nAppendRedisPipeCount = 0;

        g_logger.notice("%s", __FUNCTION__);

        KeepMysqlAlive();
        while (1)
        {
            snprintf(m_pBuf, 1024, "select f_id, f_group_id, f_type from d_ec_user.t_user_group where f_type = 0 limit %u, %u", uLimitOffset, uLimitCount);
            nRet = mysql_real_query(&m_mysql, m_pBuf, strlen(m_pBuf));
            if (nRet != 0)
            {
                g_logger.error("Failed to mysql_real_query errstr: %s", mysql_error(&m_mysql));
                return -1;
            }

            pRes = mysql_store_result(&m_mysql);
            if (pRes == NULL)
            {
                if (mysql_errno(&m_mysql) != 0)
                {
                    g_logger.error("Failed to mysql_store_rsult Error: %s", mysql_error(&m_mysql));
                    return -1;
                }
                else
                {
                    //没有数据了，跳出循环
                    break;
                }
            }

            nCurRowCount = mysql_num_rows(pRes);
            uLimitOffset += nCurRowCount;
            if (nCurRowCount == 0)
            {
                mysql_free_result(pRes);
                break;
            }

            while ((row = mysql_fetch_row(pRes)) != NULL)
            {
                nUserId = atoi(row[0]);
                nGroupId = atoi(row[1]);
                nType = atoi(row[2]);
                //m_pRedis->RedisCmdNoReply("sadd friended_set:%d %d", nGroupId, nUserId);
                m_pRedis->RedisPipeAppendCmd("sadd friended_set:%d %d", nGroupId, nUserId);
                if (++nAppendRedisPipeCount >= 100)
                {
                    m_pRedis->RedisPipeGetResult();
                    nAppendRedisPipeCount = 0;
                }
            }
            
            if (nAppendRedisPipeCount > 0)
            {
                m_pRedis->RedisPipeGetResult();
                nAppendRedisPipeCount = 0;
            }

            mysql_free_result(pRes);
            pRes = NULL;
        }

        return 0;
    }

    /*
     *@todo 全量拉取同事关系
     */
    int bus_inform_process::FullPullStaff()
    {
        uint32_t uLimitOffset = 0;
        uint32_t uLimitCount = 5000;
        int nUserId = 0;
        int nGroupId = 0;
        int nType = 0;
        MYSQL_RES *pRes = NULL;
        MYSQL_ROW row = NULL;
        int nCurRowCount = 0;
        int nRet = 0;
        int nAppendRedisPipeCount = 0;

        g_logger.notice("%s", __FUNCTION__);
        KeepMysqlAlive();
        while (1)
        {
            snprintf(m_pBuf, 1024, "select f_id, f_group_id, f_type from d_ec_user.t_user_group where f_type = 2 limit %u, %u", uLimitOffset, uLimitCount);
            nRet = mysql_real_query(&m_mysql, m_pBuf, strlen(m_pBuf));
            if (nRet != 0)
            {
                g_logger.error("Failed to mysql_real_query errstr: %s", mysql_error(&m_mysql));
                break;
            }
            pRes = mysql_store_result(&m_mysql);
            if (pRes == NULL)
            {
                if (mysql_errno(&m_mysql) != 0)
                {
                    g_logger.error("Failed to mysql_store_rsult Error: %s", mysql_error(&m_mysql));
                    return -1;
                }
                else
                {
                    return 0;
                }
            }

            nCurRowCount = mysql_num_rows(pRes);
            uLimitOffset += nCurRowCount;
            if (nCurRowCount == 0)
            {
                mysql_free_result(pRes);
                break;
            }

            while ((row = mysql_fetch_row(pRes)) != NULL)
            {
                nUserId = atoi(row[0]);
                nGroupId = atoi(row[1]);
                nType = atoi(row[2]);
                //m_pRedis->RedisCmdNoReply("sadd group_staff_set:%d %d", nGroupId, nUserId);
                //m_pRedis->RedisCmdNoReply("set user_groupid:%d %d", nUserId, nGroupId);

                m_pRedis->RedisPipeAppendCmd("sadd group_staff_set:%d %d", nGroupId, nUserId);
                m_pRedis->RedisPipeAppendCmd("set user_groupid:%d %d", nUserId, nGroupId);
                if (++nAppendRedisPipeCount >= 100)
                {
                    m_pRedis->RedisPipeGetResult();
                    nAppendRedisPipeCount = 0;
                }
            }
            if (nAppendRedisPipeCount > 0)
            {
                m_pRedis->RedisPipeGetResult();
                nAppendRedisPipeCount = 0;
            }

            mysql_free_result(pRes);
            pRes = NULL;
        }

        return 0;
    }

/*
 * set  corp_cs_set:[corpid] userid...
 * kv   cs_corpid:[userid] corpid
 * kv   cs_role:[userid] role
 * set  cs_scheme_set:[userid] scheme...
 */
    int bus_inform_process::FullPullOnlineCs()
    {
        uint32_t uLimitOffset = 0;
        uint32_t uLimitCount = 5000;
        int nCsId = 0;
        int nCorpId = 0;
        int nScheme = 0;
        MYSQL_RES *pRes = NULL;
        MYSQL_ROW row = NULL;
        int nCurRowCount = 0;
        int nRet = 0;
        int nAppendRedisPipeCount = 0;

        g_logger.notice("%s", __FUNCTION__);
        KeepMysqlAlive();
        while (1)
        {
            snprintf(m_pBuf, 1024, "select f_cs_id, f_corp_id, f_scheme from d_ec_cs.t_online_cs limit %u, %u", uLimitOffset, uLimitCount);
            nRet = mysql_real_query(&m_mysql, m_pBuf, strlen(m_pBuf));
            if (nRet != 0)
            {
                g_logger.error("Failed to mysql_real_query errstr: %s", mysql_error(&m_mysql));
                break;
            }

            pRes = mysql_store_result(&m_mysql);
            if (pRes == NULL)
            {
                if (mysql_errno(&m_mysql) != 0)
                {
                    g_logger.error("Failed to mysql_store_rsult Error: %s", mysql_error(&m_mysql));
                    return -1;
                }
                else
                {
                    return 0;
                }
            }

            nCurRowCount = mysql_num_rows(pRes);
            uLimitOffset += nCurRowCount;
            if (nCurRowCount == 0)
            {
                mysql_free_result(pRes);
                break;
            }

            while ((row = mysql_fetch_row(pRes)) != NULL)
            {
                nCsId = atoi(row[0]);
                nCorpId = atoi(row[1]);
                nScheme = atoi(row[2]);
                if (nCsId == 0 || nCorpId == 0)
                    continue;

                //m_pRedis->RedisCmdNoReply("set cs_corpid:%d %d", nCsId, nCorpId);
                //m_pRedis->RedisCmdNoReply("set cs_role:%d %d", nCsId, 0);
                //m_pRedis->RedisCmdNoReply("sadd cs_scheme_set:%d %d", nCsId, nScheme);
                //m_pRedis->RedisCmdNoReply("sadd corp_cs_set:%d %d", nCorpId, nCsId);

                m_pRedis->RedisPipeAppendCmd("set cs_corpid:%d %d", nCsId, nCorpId);
                //m_pRedis->RedisPipeAppendCmd("set cs_role:%d %d", nCsId, 0);
                m_pRedis->RedisPipeAppendCmd("sadd cs_scheme_set:%d %d", nCsId, nScheme);
                m_pRedis->RedisPipeAppendCmd("sadd corp_cs_set:%d %d", nCorpId, nCsId);

                if (++nAppendRedisPipeCount >= 100)
                {
                    m_pRedis->RedisPipeGetResult();
                    nAppendRedisPipeCount = 0;
                }
            }
            
            if (nAppendRedisPipeCount > 0)
            {
                m_pRedis->RedisPipeGetResult();
                nAppendRedisPipeCount = 0;
            }
            mysql_free_result(pRes);
            pRes = NULL;
        }

    
        return 0;
    }

    int bus_inform_process::FullPullOnlineCsLeader()
    {
        uint32_t *puCorpid = (uint32_t *)(m_pBuf + 1024 * 4);
        uint32_t uCorpidCount = 0;
        uint32_t uLimitOffset = 0;
        uint32_t uLimitCount = 5000;
        int nRet = 0;

        g_logger.notice("%s", __FUNCTION__);
        while (1)
        {
            nRet = GetCorpid(puCorpid, &uCorpidCount, uLimitOffset, uLimitCount);
            if (nRet == -1)
                return -1;
            if (uCorpidCount == 0)
                break;

            uLimitOffset += uCorpidCount;

            for (uint32_t uCorpidIndex = 0; uCorpidIndex < uCorpidCount; uCorpidIndex++)
            {
                GetCSManagerAndCreater(puCorpid[uCorpidIndex]);
            }
        }

        return 0;
    }

    int bus_inform_process::IncrProcessFriendedAndStaff(row_t *row)
    {
        if (row->size() != 3)
        {
            g_logger.error("row->size() != 3, return, %d", row->size());
            return -1;
        }

        char *p = NULL;
        int nUserId = 0;
        int nGroupId = 0;
        int nType = 0;
        int nOldUserId = 0;
        int nOldGroupId = 0;
        int nOldType = 0;

        if (row->get_value(0, &p))
            nUserId = atoi(p);
        else
        {
            g_logger.error("row->get_value() error, p:%p", p);
            return -1;
        }

        if (row->get_value(1, &p))
            nGroupId = atoi(p);
        else 
        {
            g_logger.error("row->get_value() error, p:%p", p);
            return -1;
        }

        if (row->get_value(2, &p))
            nType = atoi(p);
        else
        {
            g_logger.error("row->get_value() error, p:%p", p);
            return -1;
        }


        //printf("newvalue, userid:%d, type:%d, groupid:%d\n", nUserId, nType, nGroupId);
        g_logger.notice("newvalue, userid:%d, type:%d, groupid:%d", nUserId, nType, nGroupId);
        if (nType == 0)
        {
            //好友关系
            if (row->get_action() == 0)
            {
                m_pRedis->RedisCmdNoReply("sadd friended_set:%d %d", nGroupId, nUserId);
            }
            else if (row->get_action() == 1)
            {
                if (row->get_old_value(0, &p))
                    nOldUserId = atoi(p);
                else
                    return -1;
                if (row->get_old_value(1, &p))
                    nOldGroupId = atoi(p);
                else
                    return -1;
                if (row->get_old_value(2, &p))
                    nOldType = atoi(p);
                else
                    return -1;

                //printf("oldvalue, userid:%d, type:%d, groupid:%d\n", nOldUserId, nOldType, nOldGroupId);
                g_logger.notice("oldvalue, userid:%d, type:%d, groupid:%d", nOldUserId, nOldType, nOldGroupId);
                if (nType == nOldType)
                {
                    m_pRedis->RedisCmdNoReply("srem friended_set:%d %d", nOldGroupId, nOldUserId);
                    m_pRedis->RedisCmdNoReply("sadd friended_set:%d %d", nGroupId, nUserId);
                }
            }
            else if (row->get_action() == 2)
            {
                m_pRedis->RedisCmdNoReply("srem friended_set:%d %d", nGroupId, nUserId);
            }
        }
        else if (nType == 2)
        {
            //同事关系
            if (row->get_action() == 0)
            {
                m_pRedis->RedisCmdNoReply("sadd group_staff_set:%d %d", nGroupId, nUserId);
                m_pRedis->RedisCmdNoReply("set user_groupid:%d %d", nUserId, nGroupId);
            }
            else if (row->get_action() == 1)
            {
                if (row->get_old_value(0, &p))
                    nOldUserId = atoi(p);
                else
                {
                    g_logger.error("row->get_old_value() error, p:%p", p);
                    return -1;
                }
                if (row->get_old_value(1, &p))
                    nOldType = atoi(p);
                else 
                {
                    g_logger.error("row->get_old_value() error, p:%p", p);
                    return -1;
                }
                if (row->get_old_value(2, &p))
                    nOldGroupId = atoi(p);
                else 
                {
                    g_logger.error("row->get_old_value() error, p:%p", p);
                    return -1;
                }

                //printf("oldvalue, userid:%d, type:%d, groupid:%d\n", nOldUserId, nOldType, nOldGroupId);
                g_logger.notice("oldvalue, userid:%d, type:%d, groupid:%d", nOldUserId, nOldType, nOldGroupId);

                if (nType == nOldType)
                {
                    m_pRedis->RedisCmdNoReply("srem group_staff_set:%d %d", nGroupId, nUserId);
                    m_pRedis->RedisCmdNoReply("del user_groupid:%d %d", nUserId, nGroupId);
                    m_pRedis->RedisCmdNoReply("sadd group_staff_set:%d %d", nGroupId, nUserId);
                    m_pRedis->RedisCmdNoReply("set user_groupid:%d %d", nUserId, nGroupId);
                }

            }
            else if (row->get_action() == 2)
            {
                m_pRedis->RedisCmdNoReply("srem group_staff_set:%d %d", nGroupId, nUserId);
                m_pRedis->RedisCmdNoReply("del user_groupid:%d %d", nUserId, nGroupId);
            }
        }

        return 0;
    }

    int bus_inform_process::IncrProcessOnlineCs(row_t *row)
    {
        if (row->size() != 3)
        {
            g_logger.error("row->size() != 3, %d", row->size());
            return -1;
        }

        char *p = NULL;
        int nUserId = 0;
        int nCorpId = 0;
        int nScheme = 0;

        if (row->get_value(0, &p))
            nUserId = atoi(p);
        else
        {
            g_logger.error("row->get_value() error, p:%p", p);
            return -1;
        }

        if (row->get_value(1, &p))
            nCorpId = atoi(p);
        else 
        {
            g_logger.error("row->get_value() error, p:%p", p);
            return -1;
        }

        if (row->get_value(2, &p))
            nScheme = atoi(p);
        else
        {
            g_logger.error("row->get_value() error, p:%p", p);
            return -1;
        }


        //printf("newvalue, userid:%d, corpid:%d, scheme:%d\n", nUserId, nCorpId, nScheme);
        g_logger.notice("newvalue, userid:%d, corpid:%d, scheme:%d", nUserId, nCorpId, nScheme);
        if (row->get_action() == 0)
        {
            //新增
            m_pRedis->RedisCmdNoReply("set cs_corpid:%d %d", nUserId, nCorpId);
            //m_pRedis->RedisCmdNoReply("set cs_role:%d %d", nUserId, 0);
            m_pRedis->RedisCmdNoReply("sadd cs_scheme_set:%d %d", nUserId, nScheme);
            m_pRedis->RedisCmdNoReply("sadd corp_cs_set:%d %d", nCorpId, nUserId);
        }
        else if (row->get_action() == 1)
        {
            //更新: 业务上来说不需要处理
        }
        else if (row->get_action() == 2)
        {
            //删除
            m_pRedis->RedisCmdNoReply("srem cs_scheme_set:%d %d", nUserId, nScheme);
            m_pRedis->RedisCmdWithReply("scard cs_scheme_set:%d", nUserId);
            redisReply *pReply = m_pRedis->GetReply();
            
            if (pReply->integer == 0)
            {
                m_pRedis->RedisCmdNoReply("del cs_corpid:%d", nUserId);
                //m_pRedis->RedisCmdNoReply("del cs_role:%d", nUserId);
                m_pRedis->RedisCmdNoReply("del cs_scheme_set:%d", nUserId);
                m_pRedis->RedisCmdNoReply("srem corp_cs_set:%d %d", nCorpId, nUserId);
            }
        }

        return 0;
    }

    int bus_inform_process::IncrProcessCsLeader(row_t *row)
    {
        if (row->size() != 2)
        {
            g_logger.error("row->size() :%d", row->size());
            return -1;
        }

        char *p = NULL;
        int nUserId = 0;
        int nRoleId = 0;
        int nCorpId = 0;

        if (row->get_value(0, &p))
            nUserId = atoi(p);
        else
        {
            g_logger.error("row->get_value() error, p:%p", p);
            return -1;
        }

        if (row->get_value(1, &p))
            nRoleId = atoi(p);
        else 
        {
            g_logger.error("row->get_value() error, p:%p", p);
            return -1;
        }

        //printf("newvalue, userid:%d, roleid:%d\n", nUserId, nRoleId);
        g_logger.notice("newvalue, userid:%d, roleid:%d", nUserId, nRoleId);

        if (nRoleId != 7 && nRoleId != 9)
            return 0;

        GetCorpidByUserid(nUserId, &nCorpId);

        if (nCorpId == 0)
        {
            g_logger.error("error corpid:%d", nCorpId);
            return 0;
        }

        if (row->get_action() == 0)
        {
            m_pRedis->RedisCmdNoReply("set cs_corpid:%d %d", nUserId, nCorpId);
            m_pRedis->RedisCmdNoReply("set cs_role:%d %d", nUserId, nRoleId);
            m_pRedis->RedisCmdNoReply("sadd corp_csleader_set:%d %d", nCorpId, nUserId);
        }
        else if (row->get_action() == 1)
        {
        }
        else if (row->get_action() == 2)
        {
            m_pRedis->RedisCmdNoReply("del cs_corpid:%d %d", nUserId);
            m_pRedis->RedisCmdNoReply("del cs_role:%d %d", nUserId);
            m_pRedis->RedisCmdNoReply("srem corp_csleader_set:%d %d", nCorpId, nUserId);
        }

        return 0;
    }

    int bus_inform_process::GetCorpid(uint32_t *puCorpid, uint32_t *puCorpidCount, uint32_t uLimitOffset, uint32_t uLimitCount)
    {
        int nRet = 0;
        int nCurRowCount = 0;
        MYSQL_RES *pRes = NULL;
        MYSQL_ROW row = NULL;

        KeepMysqlAlive();
        snprintf(m_pBuf, 1024, "select f_corp_id from d_ec_corp.t_corp_info limit %u, %u", uLimitOffset, uLimitCount);
        nRet = mysql_real_query(&m_mysql, m_pBuf, strlen(m_pBuf));
        if (nRet != 0)
        {
            g_logger.error("mysql_real_query error.errstr:%s", mysql_error(&m_mysql));
            return -1;
        }

        pRes = mysql_store_result(&m_mysql);
        if (pRes == NULL)
        {
            if (mysql_errno(&m_mysql) != 0)
            {
                g_logger.error("Failed to mysql_store_rsult Error: %s", mysql_error(&m_mysql));
                return -1;
            }
            else
            {
                return 0;
            }
        }

        *puCorpidCount = mysql_num_rows(pRes);
        if (*puCorpidCount == 0)
        {
            mysql_free_result(pRes);
            return 0;
        }

        while ((row = mysql_fetch_row(pRes)) != NULL)
        {
            puCorpid[nCurRowCount] = atoi(row[0]);
            nCurRowCount++;
        }

        mysql_free_result(pRes);
        pRes = NULL;

        return *puCorpidCount;
    }

    int bus_inform_process::GetCSManagerAndCreater(uint32_t uCorpId)
    {
        int nRet = 0;
        MYSQL_RES *pRes = NULL;
        MYSQL_ROW row = NULL;
        int nCsId = 0;
        int nRoleId = 0;
        int nAppendRedisPipeCount = 0;

        KeepMysqlAlive();

        snprintf(m_pBuf, 
                1024, 
                "select a.f_id, a.f_group_id, b.f_role_id from d_ec_user.t_user_group a INNER JOIN d_ec_role.t_account_role b on a.f_id = b.f_user_id where a.f_group_id = %u and a.f_type = 2 and b.f_role_id in (7, 9)", 
                uCorpId);

        nRet = mysql_real_query(&m_mysql, m_pBuf, strlen(m_pBuf));
        if (nRet != 0)
        {
            g_logger.error("mysql_real_query error.errstr:%s", mysql_error(&m_mysql));
            return -1;
        }

        pRes = mysql_store_result(&m_mysql);
        if (pRes == NULL)
        {
            if (mysql_errno(&m_mysql) != 0)
            {
                g_logger.error("Failed to mysql_store_rsult Error: %s", mysql_error(&m_mysql));
                return -1;
            }
            else
            {
                return 0;
            }
        }

        while ((row = mysql_fetch_row(pRes)) != NULL)
        {
            nCsId = atoi(row[0]);
            nRoleId = atoi(row[2]);
            //m_pRedis->RedisCmdNoReply("set cs_corpid:%d %d", nCsId, uCorpId);
            //m_pRedis->RedisCmdNoReply("set cs_role:%d %d", nCsId, nRoleId);
            //m_pRedis->RedisCmdNoReply("sadd corp_cs_set:%d %d", uCorpId, nCsId);

            m_pRedis->RedisPipeAppendCmd("set cs_corpid:%d %d", nCsId, uCorpId);
            m_pRedis->RedisPipeAppendCmd("set cs_role:%d %d", nCsId, nRoleId);
            m_pRedis->RedisPipeAppendCmd("sadd corp_csleader_set:%d %d", uCorpId, nCsId);
            if (++nAppendRedisPipeCount >= 100)
            {
                m_pRedis->RedisPipeGetResult();
                nAppendRedisPipeCount = 0;
            }
        }

        if (nAppendRedisPipeCount > 0)
        {
            m_pRedis->RedisPipeGetResult();
            nAppendRedisPipeCount = 0;
        }
        mysql_free_result(pRes);
        pRes = NULL;
        return 0;
    }


    int bus_inform_process::GetCorpidByUserid(int nUserId, int *pnCorpId)
    {
        int nRet = 0;
        MYSQL_RES *pRes = NULL;
        MYSQL_ROW row = NULL;

        KeepMysqlAlive();

        snprintf(m_pBuf, 1024, "select f_group_id from d_ec_user.t_user_group where f_id=%d and f_type=2", nUserId);

        nRet = mysql_real_query(&m_mysql, m_pBuf, strlen(m_pBuf));
        if (nRet != 0)
        {
            g_logger.error("mysql_real_query error.errstr:%s", mysql_error(&m_mysql));
            return -1;
        }

        pRes = mysql_store_result(&m_mysql);
        if (pRes == NULL)
        {
            if (mysql_errno(&m_mysql) != 0)
            {
                g_logger.error("Failed to mysql_store_rsult Error: %s", mysql_error(&m_mysql));
                return -1;
            }
            else
            {
                return 0;
            }
        }

        if ((row = mysql_fetch_row(pRes)) != NULL)
        {
            *pnCorpId = atoi(row[0]);
        }

        mysql_free_result(pRes);
        pRes = NULL;
        return 0;
    }

    int bus_inform_process::ConnectMysql()
    {
        if(mysql_init(&m_mysql) == NULL)
        {
            g_logger.error("Failed to initate MySQL connection");
            return -1;
        }
        char value = 1;
        mysql_options(&m_mysql, MYSQL_OPT_RECONNECT, (char *)&value);
        if(!mysql_real_connect(&m_mysql, m_szMysqlIp, m_szMysqlUserName, m_szMysqlPasswd, NULL, m_nMysqlPort, NULL, 0))
        {
            g_logger.error("Failed to connect to MySQL: Error: %s, ip:%s", mysql_error(&m_mysql), m_szMysqlIp);
            return -1;
        }
        g_logger.debug("connected mysql succeed, fd:%d, ip:%s",m_mysql.net.fd, m_szMysqlIp);

        m_bIsMysqlConnected = true;
        return 0;
    }

    int bus_inform_process::DisConnectMysql()
    {
        mysql_close(&m_mysql);
        m_bIsMysqlConnected = false;
        return 0;
    }


    int bus_inform_process::KeepMysqlAlive()
    {
        if (m_bIsMysqlConnected == true)
        {
            time_t uNow = time(0);
            if (uNow - m_uLastKeepAliveTime > 10)
            {
                mysql_ping(&m_mysql);
                m_uLastKeepAliveTime = uNow;
            }
        }
        else
        {
            ConnectMysql();
        }

        return 0;
    }

};
