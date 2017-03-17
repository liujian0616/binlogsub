#include "process.h"

Log *g_pLog = NULL;
Config *g_pConfig = NULL;

Process::Process()
{
    memset(m_szPidFile, 0, sizeof(m_szPidFile));
    m_bDaemon = false;
    m_bChecksumEnable = false;

    m_sBinlogFileName.clear();
    m_uBinlogPos = 0;
    m_pMysqlProc = NULL;
    m_pPacket = NULL;
}

Process::~Process()
{

}

int Process::Daemon()
{
    pid_t pid = 0;
    pid = fork();
    if (pid > 0)
        exit(0);

    setsid();
    umask(0);

    pid = fork();
    if (pid > 0)
        exit(0);

    int fd = open("/dev/null", O_RDONLY);
    for (int i = 0; i < 3; i++)
    {
        close(i);
        dup2(fd, i);
    }
    close(fd);

    FILE *p = NULL;
    p = fopen(m_szPidFile, "w+");
    if (p == NULL)
    {   
        return -1; 
    }   
    char szBuf[128] = {0};
    snprintf(szBuf, sizeof(szBuf), "%d", getpid());
    fwrite(szBuf, 1, strlen(szBuf), p); 
    fclose(p);

    return 0;
}

void Process::SigAction(int sig, siginfo_t *pInfo, void *pContext)
{
    DOLOG("[trace]%s, got sig %d", __FUNCTION__, sig);
    if (sig == SIGINT || sig == SIGTERM)
    {   
        exit(0);
    }
}

int Process::RegisterSignal(int nSignal)
{
    struct sigaction sa; 
    sa.sa_sigaction = Process::SigAction;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    if (sigaction(nSignal, &sa, NULL) == -1)
    {
        DOLOG("[error]%s, register signal error", __FUNCTION__);
        return -1; 
    }   
    return 0;
}

bool Process::IsProcessExist()
{
    if (access(m_szPidFile, 0) != -1)
    {
        int fd = open(m_szPidFile, O_RDONLY);
        char szBuf[128] = {0};
        read(fd, szBuf, sizeof(szBuf));
        close(fd);
        pid_t pid = atol(szBuf);
        if (kill (pid, 0) == 0)
        {
            DOLOG("[trace]%s process already exist");
            return true;
        }
    }

    return false;
}

int Process::Init(char *szPidFile, bool bDaemon)
{
    int nRet = 0;

    strncpy(m_szPidFile, szPidFile, sizeof(m_szPidFile));
    m_bDaemon = bDaemon;

    if (IsProcessExist())
        return -1;

    if (m_bDaemon)
        Daemon();

    RegisterSignal(SIGINT);
    RegisterSignal(SIGTERM);


    m_pMysqlProc = new MysqlProcess();
    if (m_pMysqlProc == NULL)
        return -1;

    nRet = m_pMysqlProc->Connect();
    if (nRet != 0)
        return -1;


    m_pPacket = new Packet();
    if (m_pPacket == NULL)
        return -1;
    
    m_pMysqlProc->GetNowBinlogPos(m_sBinlogFileName, m_uBinlogPos);
    m_bChecksumEnable = m_pMysqlProc->CheckIsChecksumEnable();
    DOLOG("[trace]%s binlogfilename:%s, binlogpos:%u, bChecksumEnable:%d", __FUNCTION__, m_sBinlogFileName.c_str(), m_uBinlogPos, m_bChecksumEnable);

    IBusiness *pIBusiness = NULL;
    Business *pBusiness = NULL;
    pBusiness = new Business();
    if (pBusiness == NULL)
        return -1;
    pBusiness->Init();
    pIBusiness = pBusiness;

    nRet = m_pPacket->Init(m_bChecksumEnable, pIBusiness);
    return nRet;
}

int Process::Run()
{
    int nRet = 0;
    //DOLOG("[trace]%s bchecksumEnable:%d", __FUNCTION__, m_bChecksumEnable);
    //string sBinlogFileName;
    //uint32_t uBinlogPos;


    //strncpy(m_szBinlogFileName, "mysql-bin.000028", sizeof(m_szBinlogFileName));
    //m_uBinlogPos = 4;
    //m_uBinlogPos = 521647552;
    m_pMysqlProc->ReqBinlog(m_sBinlogFileName, m_uBinlogPos, m_bChecksumEnable);
    
    Loop();

    return 0;
}

int Process::Loop()
{
    int fd = m_pMysqlProc->GetFd();
    int nRet = 0;
    while (1)
    {
        m_pPacket->Read(fd);
        nRet = m_pPacket->Parse();
        if (nRet == -1)
            return -1;
    }
    return 0;
}


int main(int argc, char **argv)
{
    char szPidFile[] = "binlogsub.pid";
    char szConfigFile[] = "binlogsub.ini";
    char szLogPrefix[] = "binlogsub";
    bool bDaemon = false;
    int nRet = 0;

    for (int i = 0; i < argc; i++)
    {
        if (strstr(argv[i], "-daemon"))
            bDaemon = true;
    }

    g_pLog = new Log();
    if (g_pLog == NULL)
        return -1;
    g_pLog->Init(szLogPrefix);

    g_pConfig = new Config(szConfigFile);
    if (g_pConfig == NULL)
        return -1;
    g_pConfig->Load();


    Process *p = new Process();
    nRet = p->Init(szPidFile, bDaemon);
    if (nRet == 0)
        p->Run();

    return 0;
}
