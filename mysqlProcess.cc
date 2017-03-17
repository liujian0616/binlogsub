#include "mysqlProcess.h"

MysqlProcess::MysqlProcess()
{
}

MysqlProcess::~MysqlProcess()
{

}

int MysqlProcess::Connect()
{
    string sIp = g_pConfig->GetConfigStr("mysql", "mysql_ip", "");
    int nPort = g_pConfig->GetConfigInt("mysql", "mysql_port", 0);
    string sUserName = g_pConfig->GetConfigStr("mysql", "username", "");
    string sPasswd = g_pConfig->GetConfigStr("mysql", "password", "");
    int nPasswdNeedDecode = g_pConfig->GetConfigInt("mysql", "password_need_decode", 0);

    string sFinalPasswd;

    //解密password
    if (nPasswdNeedDecode != 0)
    {
        int iLen = 64;
        unsigned char pSwap[64] = {0};
        unsigned char szPassword[512] = {0};
        //Base64Decode((unsigned char *)szPasswd, strlen(szPasswd), pSwap, &iLen);
        Base64Decode((unsigned char *)sPasswd.c_str(), sPasswd.size(), pSwap, &iLen);
        uint32_t pOutlen = 0;
        if(!DesEcDncrypt(pSwap, iLen, szPassword, pOutlen, (unsigned char *)"WorkECJol"))
        {
            DOLOG("[error]%s Get mysql password error %s", __FUNCTION__, sPasswd.c_str());
            return false;
        }
        sFinalPasswd.assign((const char *)szPassword);
    }
    else
    {
        sFinalPasswd = sPasswd;
    }


    if (mysql_init(&m_mysql) == NULL)
    {
        DOLOG("[error]%s failed to init mysql connection", __FUNCTION__);
        return -1;
    }
    char value = 1;
    mysql_options(&m_mysql, MYSQL_OPT_RECONNECT, &value);

    if (mysql_real_connect(&m_mysql, sIp.c_str(), sUserName.c_str(), sFinalPasswd.c_str(), NULL, nPort, NULL, 0) == NULL)
    {
        DOLOG("[error]%s failed to mysql_read_connect, ip:%s, err:%s", __FUNCTION__, sIp.c_str(), mysql_error(&m_mysql));
        return -1;
    }
    
    DOLOG("[trace]%s connected mysql succeed. fd:%d", __FUNCTION__, m_mysql.net.fd);
    m_bConnected = true;
    return 0;
}

int MysqlProcess::DisConnect()
{
    m_bConnected = false;
    mysql_close(&m_mysql);
    return 0;
}

int MysqlProcess::KeepAlive()
{
    if (m_bConnected == true)
    {
        time_t tNow = time(0);
        if (m_tLastKeepAliveTime + 10 < tNow)
        {
            mysql_ping(&m_mysql);
            m_tLastKeepAliveTime = tNow;
        }
    }
    else
    {
        Connect();
    }

    return 0;
}


bool MysqlProcess::CheckIsChecksumEnable()
{
    mysql_query(&m_mysql, "SHOW GLOBAL VARIABLES LIKE 'BINLOG_CHECKSUM'");
    MYSQL_RES *pRes = mysql_store_result(&m_mysql);
    if (pRes == NULL)
    {
        DOLOG("[error]%s Failed to mysql_store_rsult Error: %s", __FUNCTION__, mysql_error(&m_mysql));
        return false;
    }
    MYSQL_ROW row = mysql_fetch_row(pRes);
    if (row == NULL)
    {
        DOLOG("[error]%s mysql_fetch_row get empty row", __FUNCTION__);
        mysql_free_result(pRes);
        return false;
    }
    if (strcmp(row[1], "NONE") == 0)
    {
        mysql_free_result(pRes);
        return false;
    }

    mysql_free_result(pRes);
    return true;
}


int MysqlProcess::GetNowBinlogPos(string &sBinlogFileName, uint32_t &uBinlogPos)
{
#if 0
    mysql_query(&m_mysql, "show master status");
    MYSQL_RES *pRes = mysql_store_result(&m_mysql);
    if (pRes == NULL)
    {
        DOLOG("[error]%s Failed to mysql_store_rsult Error: %s", __FUNCTION__, mysql_error(&m_mysql));
        return -1;
    }
    MYSQL_ROW row = mysql_fetch_row(pRes);
    if (row == NULL)
    {
        DOLOG("[error]%s mysql_fetch_row get empty row", __FUNCTION__);
        return -1;
    }
    strncpy(szBinlogFileName, row[0], 100);
    *puBinlogPos = atol(row[1]);

    //mysql_free_result(pRes);

    DOLOG("[trace]%s get now binlog pos: %s,%d", __FUNCTION__, szBinlogFileName, *puBinlogPos);
#endif

    mysql_query(&m_mysql, "show master status");
    MYSQL_RES *pRes = mysql_store_result(&m_mysql);
    if (pRes == NULL)
    {
        DOLOG("[error]%s Failed to mysql_store_rsult Error: %s", __FUNCTION__, mysql_error(&m_mysql));
        return -1;
    }
    MYSQL_ROW row = mysql_fetch_row(pRes);
    if (row == NULL)
    {
        DOLOG("[error]%s mysql_fetch_row get empty row", __FUNCTION__);
        mysql_free_result(pRes);
        return -1;
    }
    
    sBinlogFileName.assign(row[0]);
    uBinlogPos = atol(row[1]);

    mysql_free_result(pRes);
    return 0;
}


int MysqlProcess::GetFd()
{
    return m_mysql.net.fd;
}


int MysqlProcess::ReqBinlog(string &sBinlogFileName, uint32_t uBinlogPos, bool bChecksumEnable)
{
    int nRet = 0;
    int fd = -1;
    int flags = 0;
    fd = m_mysql.net.fd;

    if (bChecksumEnable)
    {
        mysql_query(&m_mysql, "SET @master_binlog_checksum='NONE'");
        //mysql_query(&m_mysql, "SET @master_binlog_checksum=@@global.binlog_checksum");
    }


    if ((flags = fcntl(fd, F_GETFL)) == -1) 
    {
        DOLOG("[error]%s fcntl(F_GETFL): %s", __FUNCTION__, strerror(errno));
        return -1;
    }

    flags &= ~O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) == -1) 
    {
        DOLOG("[error]%s fcntl(F_SETFL,O_BLOCK): %s", __FUNCTION__, strerror(errno));
        return -1;
    }

    /*
     *@todo 组包发送到mysql，请求binlog
     *包结构为  length      3
     *          seq         1
     *          cmd         1 
     *          binlogpos   4
     *          flags       2 
     *          serverid    4
     *          binlogfilename    总长度-前面的定长
     */
    char szBuf[1024] = {0};
    uint32_t uBodyLen = 0;
    uint32_t uOffset = 4;
    uint8_t uCmd = 0x12;
    uint16_t uFlags = 0x00;
    uint32_t uServerId = g_pConfig->GetConfigInt("mysql", "mysql_serverid", 0);

    szBuf[uOffset] = uCmd;
    uBodyLen += 1;
    int4store(szBuf + uOffset + uBodyLen, uBinlogPos);
    uBodyLen += 4;

    int4store(szBuf + uOffset + uBodyLen, uFlags);
    uBodyLen += 2;

    int4store(szBuf + uOffset + uBodyLen, uServerId);
    uBodyLen += 4;

    uint32_t uFileNameLen = sBinlogFileName.size();
    memcpy(szBuf + uOffset + uBodyLen, sBinlogFileName.c_str(), uFileNameLen);
    uBodyLen += uFileNameLen; 

    szBuf[uOffset - 1] = 0x00; //seq
    int3store(szBuf, uBodyLen);

    nRet = write(m_mysql.net.fd, szBuf, uBodyLen+4);
    if (nRet != int(uBodyLen+4))
    {
        DOLOG("[error]%s write dump cmd packet fail, error:%s", __FUNCTION__, strerror(errno));
        return -1;
    }

    DOLOG("[trace]%s reqbinlog, file:%s, logpos:%lu", __FUNCTION__, sBinlogFileName.c_str(), uBinlogPos);

    return 0;
}
