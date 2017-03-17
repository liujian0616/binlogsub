#ifndef MYSQLPROCESS_H
#define MYSQLPROCESS_H

#include <fcntl.h>
#include <mysql/mysql.h>
#include <errno.h>
#include "config.h"
#include "log.h"
#include "myconvert.h"
#include "util.h"

class MysqlProcess
{
    public:
        MysqlProcess();
        ~MysqlProcess();

        /*  
         *@todo 连接数据库
         */
        int Connect();

        /*  
         *@todo 断开数据库连接
         */
        int DisConnect();

        /*  
         *@todo 保持数据库连接
         */
        int KeepAlive();

        /*  
         *@todo 判断与mysql交互的协议是否启用了checksum，mysql5.6新加入checksum机制
         */
        bool CheckIsChecksumEnable();


        /*
         *@todo 获取到数据库中binlog的最新位置
         *@param szBinlogFileName binlog文件名
         *@param puBinlogPos binlog文件的偏移量
         */
        int GetNowBinlogPos(string &sBinlogFileName, uint32_t &uBinlogPos);


        /*
         *@todo 获取mysql的文件描述符
         */
        int GetFd();

        /*  
         *@todo 向数据库服务器发送数据报,请求服务器将binlog发过来
         */
        int ReqBinlog(string &sBinlogFileName, uint32_t uBinlogPos, bool bChecksumEnable);


    private:
        MYSQL   m_mysql;
        time_t  m_tLastKeepAliveTime;
        bool    m_bConnected;

};

#endif
