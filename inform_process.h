#ifndef INFORM_PROCESS_H
#define INFORM_PROCESS_H

#include <unistd.h>
#include <netdb.h>
#include <mysql/mysql.h>
#include "myredis.h"
#include "bus/bus_user_process.h"
#include "bus/bus_config.h"


namespace bus
{
/*
 *@todo 
 * 被好友信息redis结构 
 * 同事
 *  set   group_staff_set:[corpid]  userid...
 *  kv    user_groupid:[userid] corpid
 * 好友
 *  friended_set:[friendedid] userid
 * 访客信息redis存储结构
 * set  corp_cs_set:[corpid] userid...          //企业id对应的客服id集合
 * kv   cs_corpid:[userid] corpid               //用户id对应的企业id
 * set  cs_scheme_set:[userid] scheme...        //客服套数
 *访客经理和企业创始人
 * set  corp_csleader_set:[corpid] userid...    //企业id对应的客服经理集合
 * kv   cs_corpid:[userid] corpid               //用户id对应的企业id
 * kv   cs_role:[userid] role            7-企业创始人 9-客服经理
 */


/*
 *@todo 以下为老结构,访客经理和访客放在一起,现在因为别的业务需要，将他们分开存
 * set  corp_cs_set:[corpid] userid...          //企业id对应的客服id集合
 * kv   cs_corpid:[userid] corpid               //用户id对应的企业id
 * set  cs_scheme_set:[userid] scheme...        //客服套数
 * kv   cs_role:[userid] role          0-普通客服  7-企业创始人 9-客服经理

 */
    class bus_inform_process : public bus_user_process
    {
        public:
            bus_inform_process();
            virtual ~bus_inform_process();

            int Init(bus_config_t *pConfig);
            int Init(const char *szMysqlIp, int nMysqlPort, const char *szMysqlUserName, const char *szMysqlPasswd, bool bPasswdNeedDecode, const char *szRedisIp, int nRedisPort, bool bRedisNeedPasswd, const char *szRedisPasswd);

            virtual int FullPull();
            virtual int IncrProcess(row_t *row);
            virtual int SaveNextreqPos(const char *szBinlogFileName, uint32_t uBinlogPos);
            virtual int ReadNextreqPos(char *szBinlogFileName, uint32_t uFileNameLen, uint32_t *puBinlogPos);


            int FullPullFriended();
            int FullPullStaff();
            int FullPullOnlineCs();
            int FullPullOnlineCsLeader();

            int IncrProcessFriendedAndStaff(row_t *row);
            int IncrProcessOnlineCs(row_t *row);
            int IncrProcessCsLeader(row_t *row);

            int GetCorpid(uint32_t *puCorpid, uint32_t *puCorpidCount, uint32_t uLimitOffset, uint32_t uLimitCount);
            int GetCSManagerAndCreater(uint32_t uCorpId);
            int GetCorpidByUserid(int nUserId, int *pnCorpId);

            int ConnectMysql();
            int DisConnectMysql();
            int KeepMysqlAlive();

        private:
            MYSQL       m_mysql;                        //mysql连接
            char        m_szMysqlIp[16];                //mysqlip
            int         m_nMysqlPort;                   //mysql端口
            char        m_szMysqlUserName[24];          //mysql用户名
            char        m_szMysqlPasswd[48];            //mysql密码
            bool        m_bIsMysqlConnected;          //数据库是否正常连接
            time_t      m_uLastKeepAliveTime;           //上次执行 mysql ping的时间
            

            MyRedis     *m_pRedis;
            char        *m_pBuf;


    };
}
#endif
