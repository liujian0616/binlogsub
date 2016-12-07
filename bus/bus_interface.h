#ifndef BUS_INTERFACEH
#define BUS_INTERFACEH

#include <mysql/mysql.h>
#include "bus_config.h"
#include "bus_row.h"
#include "bus_user_process.h"

namespace bus
{
    class bus_packet_t;

    class bus_interface
    {
        public:
            bus_interface();

            virtual ~bus_interface();

            /*
             *@todo 初始化，读取配置文件，将配置存到bus空间的全局变量中
             *@param szIp mysql服务器的 ip或者域名
             *@param nPort mysql服务器端口
             *@param szUserName mysql用户名
             *@param szPasswd mysql密码
             *@param mysql服务器的 serverid,mysql服务器用这个编号防止binlog消息循环解析 
             */
            int Init(const char *szIp, int nPort, const char *szUserName, const char *szPasswd, int nServerid, bool bPasswdNeedDecode, bus_user_process *pUserProcess);

            int Init(bus_config_t *pConfig, bus_user_process *pUserProcess);

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
             *@todo 向数据库服务器发送数据报,请求服务器将binlog发过来
             */
            int ReqBinlog();

            /*
             *@todo 判断与mysql交互的协议是否启用了checksum，mysql5.6新加入checksum机制
             */
            bool CheckIsChecksumEnable();


            /*
             *@todo 读取binlog，解析
             *@param pack,负责读取和解析binlog包的类
             *@param cb,解析到符合要求的包后调用的回调函数
             */
            int ReadAndParse(bus_user_process *pUserProcess);

            /*
             *@todo 获取到数据库中binlog的最新位置
             *@param szBinlogFileName binlog文件名
             *@param puBinlogPos binlog文件的偏移量
             */
            int GetNowBinlogPos(char *szBinlogFileName, uint32_t *puBinlogPos);

            /*
             *@todo 设置 mysqlbinlog的解析起始位置
             */
            int SetIncrUpdatePos(const char *szBinlogFileName, uint32_t uBinlogPos);
            //int SetBinlogFileName(char *szBinlogFileName);
            //int SetBinlogPos(uint32_t uBinlogPos);
            int UpdateToNextPos();




        private: 
            MYSQL       m_mysql;                        //mysql连接
            char        m_szConfigFile[100];            //配置文件的路径
            char        m_szLogFilePrefix[100];         //配置文件名的前缀
            char        m_szIp[16];                     //mysqlip
            int         m_nPort;                        //mysql端口
            char        m_szUserName[24];               //mysql用户名
            char        m_szPasswd[48];                 //mysql密码
            int         m_nServerid;                    //数据库的serverid,mysql用它防止解析自身数据库产生的binlog，防止binlog循环解析
            char        m_szBinlogFileName[100];        //binlog文件名
            uint32_t    m_uBinlogPos;                   //binlog偏移位置
            bus_packet_t    *m_pPacket;                 //binlog包对象，用于解析拉取到的包
            time_t      m_uLastKeepAliveTime;           //上次执行 mysql ping的时间
            bool        m_bIsConnected;                 //数据库是否正常连接
            bool        m_bIsChecksumEnable;            //mysql服务器是否启用checksum机制
            time_t      m_uLastSetPosTime;           //上次将pos记录到redis的时间
            bus_user_process *m_pUserProcess;
    };
}
#endif
