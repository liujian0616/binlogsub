#include "myregex.h"

Regex::Regex()
{
    memset(m_szPattern, 0, sizeof(m_szPattern));
}

Regex::~Regex()
{
    regfree(&m_reg);
}

int Regex::Init(const char *szPattern)
{
    if (szPattern == NULL || szPattern[0] == 0)
    {   
        return -1; 
    }   

    strncpy(m_szPattern, szPattern, sizeof(m_szPattern));
    char szErrBuffer[128] = {0};
    int nRet = regcomp(&m_reg, m_szPattern, REG_EXTENDED);
    if (nRet != 0)
    {   
        regerror(nRet, &m_reg, szErrBuffer, sizeof(szErrBuffer));
        return -1; 
    }   
    return 0;
}

bool Regex::Check(const char *szText)
{
    if (szText == NULL || szText[0] == 0)
    {   
        return false;
    }   

    size_t nmatch = 1;
    regmatch_t pmatch[1];
    int nStatus = regexec(&m_reg, szText, nmatch, pmatch, 0); 
    if (nStatus == 0)
    {   
        return true;
    }   

    return false;
}

