#ifndef LOG_H
#define LOG_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

class Log
{
    public:
        Log();
        ~Log();

        int Init(char *szPrefix);
        int DoLog(const char* fmt, ...);

    private:
        FILE *m_fp;
        char m_szPrefix[32];
        char m_szFileName[32];
        char m_szDir[64];
        char m_szFullPath[128];
        char m_szBuf[4096];
};

extern Log *g_pLog;
#define DOLOG(fmt, args...) g_pLog->DoLog(fmt, ##args);

#endif
