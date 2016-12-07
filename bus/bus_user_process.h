#ifndef USER_PROCESS_H
#define USER_PROCESS_H

#include "bus_row.h"

namespace bus
{
    /*
     * @todo 业务相关操作接口
     */
    class bus_user_process
    {
        public:
            //全量拉取业务数据
            virtual int FullPull() = 0;

            //拉取到增量数据后的回调函数
            virtual int IncrProcess(row_t *row) = 0;

            //将当前解析到binlog的位置存储
            virtual int SaveNextreqPos(const char *szBinlogFileName, uint32_t uBinlogPos) = 0;

            //读出上一次解析到的binlog的位置,用于重连继续解析
            virtual int ReadNextreqPos(char *szBinlogFileName, uint32_t uFileNameLen, uint32_t *puBinlogPos) = 0;
    };

    class bus_user_process_demo : public bus_user_process
    {
        public:
            bus_user_process_demo() 
            {
                memset(m_szBinlogFileName, 0, sizeof(m_szBinlogFileName));
                m_uBinlogPos = 0;
            }
            
            ~bus_user_process_demo(){}

            virtual int FullPull() { return 0; };

            virtual int IncrProcess(row_t *row)
            {
                char *p = NULL;
                printf("action:%d, database:%s, table:%s, column_count:%d\n", row->get_action(), row->get_db_name(), row->get_table(), row->size());
                for (uint32_t i = 0; i < row->size(); i++)
                {
                    row->get_value(i, &p);   
                    if (p != NULL)
                        printf("index:%d, value:%s\n", i, p);
                    else
                        printf("get value error\n");
                }

                return 0;
            }


            virtual int SaveNextreqPos(const char *szBinlogFileName, uint32_t uBinlogPos)
            {
                printf("save nextreq  logpos in memory, filename:%s, logpos:%u\n", szBinlogFileName, uBinlogPos);
                /*
                 *@todo 重连后请求的位置
                 */
                memset(m_szBinlogFileName, 0, sizeof(m_szBinlogFileName));
                strncpy(m_szBinlogFileName, szBinlogFileName, sizeof(m_szBinlogFileName));
                m_uBinlogPos = uBinlogPos;
                return 0;
            }

            virtual int ReadNextreqPos(char *szBinlogFileName, uint32_t uFileNameLen, uint32_t *puBinlogPos)
            {
                strncpy(szBinlogFileName, m_szBinlogFileName, uFileNameLen);
                *puBinlogPos = m_uBinlogPos;
                printf("read nextreq  logpos form memory. filename:%s, logpos:%u\n", szBinlogFileName, *puBinlogPos);
                return 0;
            }

        private:
            char        m_szBinlogFileName[64];
            uint32_t    m_uBinlogPos;
    };
}
#endif
