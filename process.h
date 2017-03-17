#ifndef PROCESS_H
#define PROCESS_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <signal.h>
#include "config.h"
#include "log.h"
#include "mysqlProcess.h"
#include "packet.h"
#include "business.h"

/*
 *@todo 抽象出一个进程
 */
#if 0
struct tagChangeUser
{
    int         nAction;    //0-新增  1-修改 2-删除
    uint64_t    nUid;
    char        szAccount[128];
};
#endif


class Process
{
    public:
        Process();
        virtual ~Process();

        int Daemon();
        static void SigAction(int sig, siginfo_t *pInfo, void *pContext);
        int RegisterSignal(int nSignal);
        bool IsProcessExist();

        int Init(char *szPidFile, bool bDaemon);
        int Run();
        int Loop();

    private:
        char m_szPidFile[128];
        bool m_bDaemon;
        bool m_bChecksumEnable;

        string m_sBinlogFileName;
        uint32_t m_uBinlogPos;

        MysqlProcess *m_pMysqlProc;
        Packet *m_pPacket;
};
#endif
