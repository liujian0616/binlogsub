#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <mysql/mysql.h>
#include "bus_event.h"
#include "bus_config.h"
#include "bus_util.h"
#include "bus_log.h"

#include "bus_interface.h"
#include "bus_userprocess.h"
#include <string>

int main(int argc, char **argv)
{
#if 0
    char szIp[16] = "192.168.1.222";
    int nPort = 3306;
    char szUserName[] = "ec";
    char szPasswd[] = "ecEC!)@(#*$*";
    int nMysqlServerid = 1;
#endif

#if 1
    char szIp[16] = "10.0.201.141";
    int nPort = 3306;
    char szUserName[] = "ecuser";
    char szPasswd[] = "ecuser";
    int nMysqlServerid = 1;
#endif

#if 0
    char szIp[16] = "10.0.102.2";
    int nPort = 3306;
    char szUserName[] = "root";
    char szPasswd[] = "resuce";
    int nMysqlServerid = 1;
#endif

#if 0
    char szIp[16] = "10.0.200.191";
    int nPort = 3323;
    char szUserName[] = "binlog";
    char szPasswd[] = "binlog";
    int nMysqlServerid = 2074685921;
#endif
    int ret = 0;



    bus::bus_user_process *pUserProcess = new bus::bus_user_process_demo();
    if (pUserProcess == NULL)
        return -1;
   
    bus::bus_interface *pInterface = new bus::bus_interface();
    ret = pInterface->Init("informHubSvr.ini", "bus", szIp, nPort, szUserName, szPasswd, nMysqlServerid, false, pUserProcess);
    if (ret != 0)
        return ret;


    char szBinlogFileName[100] = {0};
    uint32_t uBinlogPos = 0;
    pInterface->GetNowBinlogPos(szBinlogFileName, &uBinlogPos);
    pInterface->SetIncrUpdatePos(szBinlogFileName, uBinlogPos);
    //pInterface->SetIncrUpdatePos("mysql-bin.000040", 382258411);

    pUserProcess->FullPull();

    while (true)
    {
        pInterface->KeepAlive();

        ret = pInterface->ReqBinlog();
        if (ret != 0)
        {
            continue;
        }

        while (true)
        {
            ret = pInterface->ReadAndParse(pUserProcess);
            if (ret != 0)
            {
                pInterface->UpdateToNextPos();
                pInterface->DisConnect();
                sleep(1);
                break;
            }
        }
    }
    return 0;
}

