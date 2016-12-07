#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <mysql/mysql.h>
#include "bus_event.h"
#include "bus_config.h"
#include "bus_util.h"
#include "bus_log.h"

#include "bus_interface.h"
#include <string>

namespace bus
{

    //extern bus_config_t g_config;
    extern bus_log_t    g_logger;

    bus_interface::bus_interface()
    {
        memset(m_szIp, 0, sizeof(m_szIp));
        m_nPort = 0;
        memset(m_szUserName, 0, sizeof(m_szUserName));
        memset(m_szPasswd, 0, sizeof(m_szPasswd));
        m_nServerid = 0;
        memset(m_szBinlogFileName, 0, sizeof(m_szBinlogFileName));
        m_uBinlogPos = 4; //每一个binlog文件 最小有效位置是4

        m_pPacket = NULL;
        m_uLastKeepAliveTime = 0;
        m_bIsConnected = false;
        m_bIsChecksumEnable = false;

        m_pUserProcess = NULL;
    }

    bus_interface::~bus_interface()
    {
        DisConnect();
    }

    int bus_interface::Init(bus_config_t *pConfig, bus_user_process *pUserProcess)
    {
        int nRet = 0;
        nRet = Init(pConfig->_mysql_ip, pConfig->_mysql_port, pConfig->_mysql_username, pConfig->_mysql_userpasswd, pConfig->_mysql_serverid, pConfig->_password_need_decode, pUserProcess);
        return nRet;
    }

    int bus_interface::Init(const char *szIp, int nPort, const char *szUserName, const char *szPasswd, int nServerid, bool bPasswdNeedDecode, bus_user_process *pUserProcess)
    {
        int nRet = 0;


        struct hostent *pHostEnt = gethostbyname(szIp);
        struct in_addr addr;
        addr.s_addr = *(unsigned long *)pHostEnt->h_addr_list[0];
        char *szRealIp = inet_ntoa(addr);
        strncpy(m_szIp, szRealIp, sizeof(m_szIp));

        m_nPort = nPort;
        strncpy(m_szUserName, szUserName, sizeof(m_szUserName));

        //解密password
        if (bPasswdNeedDecode)
        {
            int iLen = 64;
            unsigned char pSwap[64] = {0};
            unsigned char szPassword[512] = {0};
            Base64Decode((unsigned char *)szPasswd,strlen(szPasswd),pSwap,&iLen);
            u_int pOutlen = 0;
            if(!DesEcDncrypt(pSwap,iLen,szPassword,pOutlen,(unsigned char *)"WorkECJol"))
            {
                g_logger.error("Get mysql password error %s\n",szPasswd);
                return false;
            }
            strncpy(m_szPasswd, (const char *)szPassword, sizeof(m_szPasswd));
        }
        else
        {
            strncpy(m_szPasswd, szPasswd, sizeof(m_szPasswd));
        }

        m_nServerid = nServerid;
        m_pUserProcess = pUserProcess;

        nRet = Connect();
        if (nRet == -1)
        {
            g_logger.error("connect mysql error\n");
            return -1;
        }

        CheckIsChecksumEnable();

        m_pPacket = new bus_packet_t(m_bIsChecksumEnable);
        if (m_pPacket == NULL)
        {
            g_logger.error("new bus_packet_t() error\n");
            return -1;
        }
        return 0;
    }

    int bus_interface::Connect()
    {
        if(mysql_init(&m_mysql) == NULL)
        {
            g_logger.error("Failed to initate MySQL connection");
            return -1;
        }
        char value = 1;
        mysql_options(&m_mysql, MYSQL_OPT_RECONNECT, (char *)&value);
        if(!mysql_real_connect(&m_mysql, m_szIp, m_szUserName, m_szPasswd, NULL, m_nPort, NULL, 0))
        {
            g_logger.error("Failed to connect to MySQL: Error: %s, ip:%s", mysql_error(&m_mysql), m_szIp);
            return -1;
        }
        g_logger.debug("connected mysql succeed, fd:%d, ip:%s",m_mysql.net.fd, m_szIp);

        m_bIsConnected = true;
        return 0;
    }

    int bus_interface::DisConnect()
    {
        mysql_close(&m_mysql);
        m_bIsConnected = false;
        return 0;
    }


    int bus_interface::KeepAlive()
    {
        if (m_bIsConnected == true)
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
            Connect();
        }

        return 0;
    }

    int bus_interface::ReqBinlog()
    {
        int nRet = 0;
        int fd = -1;
        int flags = 0;
        fd = m_mysql.net.fd;

        if (m_bIsChecksumEnable == true)
        {   
            mysql_query(&m_mysql, "SET @master_binlog_checksum='NONE'");
        }


        if ((flags = fcntl(fd, F_GETFL)) == -1) {
            g_logger.error("fcntl(F_GETFL): %s", strerror(errno));
            return -1;
        }
        flags &= ~O_NONBLOCK;
        if (fcntl(fd, F_SETFL, flags) == -1) {
            g_logger.error("fcntl(F_SETFL,O_BLOCK): %s", strerror(errno));
            return -1;
        }

        bus::bus_dump_cmd_t dump_cmd(m_szBinlogFileName, m_uBinlogPos, m_nServerid);
        nRet = dump_cmd.write_packet(fd);
        if (nRet != 0) {
            g_logger.error("write dump packe fail");
            return -1;
        }

        g_logger.notice("reqbinlog, file:%s, logpos:%lu", m_szBinlogFileName, m_uBinlogPos); 
        return 0;
    }

    bool bus_interface::CheckIsChecksumEnable()
    {
        mysql_query(&m_mysql, "SHOW GLOBAL VARIABLES LIKE 'BINLOG_CHECKSUM'");
        MYSQL_RES *pRes = mysql_store_result(&m_mysql);
        if (pRes == NULL)
        {
            g_logger.error("%s Failed to mysql_store_rsult Error: %s", __FUNCTION__, mysql_error(&m_mysql));
            return -1;
        }
        MYSQL_ROW row = mysql_fetch_row(pRes);
        if (row == NULL)
        {
            g_logger.error("%s mysql_fetch_row get empty row", __FUNCTION__); 
            return -1;
        }
        if (strcmp(row[1], "NONE") == 0)
        {
            m_bIsChecksumEnable = false;
            return false;
        }

        m_bIsChecksumEnable = true;
        mysql_free_result(pRes);
        return true;
    }

    int bus_interface::ReadAndParse(bus_user_process *pUserProcess)
    {
        int nRet = 0;
        int fd = m_mysql.net.fd;

        nRet = m_pPacket->read_packet(fd);
        if (nRet != 0) 
        {
            /* read error, or eof */
            g_logger.error("read error");
            return nRet;
        }

        nRet = m_pPacket->parse_packet(pUserProcess);
        if (nRet == -1) 
        {
            /* parse packet fail */
            g_logger.error("parse error");
            return nRet;
        } 
        else if (nRet == 1) 
        {
            /* stop signal */
            g_logger.debug("stop signal");
            return nRet;
        }
        else if(nRet == 2) 
        {
            /* eof */
            g_logger.debug("read eof");
            return nRet;
        }

        return 0;
    }

    int bus_interface::GetNowBinlogPos(char *szBinlogFileName, uint32_t *puBinlogPos)
    {
        mysql_query(&m_mysql, "show master status");
        MYSQL_RES *pRes = mysql_store_result(&m_mysql);
        if (pRes == NULL)
        {
            g_logger.error("Failed to mysql_store_rsult Error: %s", mysql_error(&m_mysql));
            return -1;
        }
        MYSQL_ROW row = mysql_fetch_row(pRes);
        if (row == NULL)
        {
            g_logger.error("mysql_fetch_row get empty row"); 
            return -1;
        }
        strncpy(szBinlogFileName, row[0], 100); 
        *puBinlogPos = atol(row[1]);

        mysql_free_result(pRes);

        g_logger.notice("get now binlog pos: %s:%d", szBinlogFileName, *puBinlogPos); 

        return 0;
    }

    int bus_interface::SetIncrUpdatePos(const char *szBinlogFileName, uint32_t uBinlogPos)
    {
        memset(m_szBinlogFileName, 0, sizeof(m_szBinlogFileName));
        strncpy(m_szBinlogFileName, szBinlogFileName, sizeof(m_szBinlogFileName));
        m_uBinlogPos = uBinlogPos;

        m_pUserProcess->SaveNextreqPos(m_szBinlogFileName, m_uBinlogPos);
        return 0;
    }

#if 0
    int bus_interface::SetBinlogFileName(char *szBinlogFileName)
    {
        memset(m_szBinlogFileName, 0, sizeof(m_szBinlogFileName));
        strncpy(m_szBinlogFileName, szBinlogFileName, sizeof(m_szBinlogFileName));
        return 0;
    }

    int bus_interface::SetBinlogPos(uint32_t uBinlogPos)
    {
        m_uBinlogPos = uBinlogPos;
        return 0;
    }
#endif

    int bus_interface::UpdateToNextPos()
    {
        //memset(m_szBinlogFileName, 0, sizeof(m_szBinlogFileName));
        //m_uBinlogPos = 0;
        m_pUserProcess->ReadNextreqPos(m_szBinlogFileName, sizeof(m_szBinlogFileName),  &m_uBinlogPos);
        return 0;
    }

}

