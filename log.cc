#include "log.h"

Log::Log()
{
    m_fp = NULL;
    memset(m_szPrefix, 0, sizeof(m_szPrefix));
    memset(m_szFileName, 0, sizeof(m_szFileName));
    memset(m_szDir, 0, sizeof(m_szDir));
    memset(m_szFullPath, 0, sizeof(m_szFullPath));
    memset(m_szBuf, 0, sizeof(m_szBuf));
}

Log::~Log()
{
    if (m_fp)
    {
        fclose(m_fp);
    }
}

int Log::Init(char *szPrefix)
{
    if (szPrefix == NULL)
        return -1;

    strncpy(m_szDir, "log", sizeof(m_szDir));
    strncpy(m_szPrefix, szPrefix, sizeof(m_szPrefix));
    if (access(m_szDir, 0) != 0)
    {
        if (mkdir(m_szDir, 0755) != 0)
            return -1;
    }

    time_t tNowtime = time(0);
    struct tm *pTm = localtime(&tNowtime);
    snprintf(m_szFullPath, sizeof(m_szFullPath), "%s/%s_%04d_%02d_%02d.log", m_szDir, m_szPrefix, pTm->tm_year + 1900, pTm->tm_mon + 1, pTm->tm_mday);
    return 0;
}


int Log::DoLog(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(m_szBuf, sizeof(m_szBuf), fmt, ap);
    va_end(ap);

    /*
     *@todo 按日期切换文件
     */
    time_t now = time(NULL);
    struct tm *pTm = NULL;
    struct stat st;

    stat(m_szFullPath, &st);
    pTm = localtime(&st.st_mtime);
    int oldDay = pTm->tm_mday;
    pTm = localtime(&now);
    int nowDay = pTm->tm_mday;
    if (nowDay != oldDay)
    {
        snprintf(m_szFullPath, sizeof(m_szFullPath), "%s/%s_%04d_%02d_%02d.log", m_szDir,
                m_szPrefix, pTm->tm_year + 1900, pTm->tm_mon + 1, nowDay);

        if (m_fp != NULL)
        {
            fclose(m_fp);
            if (m_szFullPath[0] != 0)
                m_fp =  fopen(this->m_szFullPath, "a+");
        }
    }

    /*
     *@todo 写日志到文件
     */
    char szTimeBuf[64] = {0};
    strftime(szTimeBuf, sizeof(szTimeBuf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    if (m_fp == NULL && m_szFullPath[0] != 0) 
    {
        m_fp =  fopen(this->m_szFullPath, "a+");
    }

    fprintf(m_fp, "%s %s\n", szTimeBuf, m_szBuf);
    fflush(m_fp);

    return 0;
}
