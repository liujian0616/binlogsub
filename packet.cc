#include "packet.h"

/***********************class Packet*******************/
int PacketBase::Init(uint32_t uBodyLen, uint8_t u8Seq, uint8_t u8ErrFlag, char *pBodyBuf)
{
    m_uBodyLen = uBodyLen;
    m_u8Seq = u8Seq;
    m_u8ErrFlag = u8ErrFlag;
    m_pBodyBuf = pBodyBuf;
    m_uPos = 1; //加上u8ErrFlag的偏移 1 byte
    return 0;
}


/***********************class ErrorPacket*******************/
int ErrorPacket::Parse()
{
    m_u16ErrCode = uint2korr(m_pBodyBuf + m_uPos);
    m_uPos += 2;

    memcpy(m_szState, m_pBodyBuf+m_uPos, sizeof(m_szState));
    m_uPos += sizeof(m_szState);

    uint32_t uLeft = m_uBodyLen- m_uPos;
    m_sInfo.assign(m_pBodyBuf+m_uPos, uLeft);
    m_uPos += uLeft;

    DOLOG("[error]%s recv error packet. errinfo:%s", __FUNCTION__, m_sInfo.c_str());
    return 0;
}

int ErrorPacket::PrintInfo()
{
    DOLOG("[error]%s error packet. errcode:%d state:%s, info:%s", 
            __FUNCTION__,
            m_u16ErrCode,
            m_szState,
            m_sInfo.c_str());
    return 0;
}

/***********************class NormalPacket*******************/

int NormalPacket::Parse()
{
    if (m_uBodyLen < 19)
    {
        DOLOG("[error]%s packet body length < 19", __FUNCTION__);
        return -1;
    }

    m_pContext->cEventCommHead.Parse(m_pBodyBuf, m_uPos);

    /*
     *@todo更新 binlog的position
     * binlog文件中中不存在的rotate event 或者fileformate event,  包体中 logpos = 0，用它减去event_sz 会造成溢出
     */
    if (m_pContext->cEventCommHead.m_uLogPos != 0)
    {
        m_pContext->sNextBinlogFileName = m_pContext->cRotateEvent.m_sNextBinlogFilename;
        m_pContext->uNextBinlogPos = m_pContext->cEventCommHead.m_uLogPos;
        DOLOG("[trace]%s eventType:0x%X, binlogName:%s, curPos:%u, nextPos:%u", 
                __FUNCTION__,
                m_pContext->cEventCommHead.m_u8EventType,
                m_pContext->sNextBinlogFileName.c_str(),
                m_pContext->uNextBinlogPos - m_uBodyLen + 1,
                m_pContext->uNextBinlogPos);
    }

    switch (m_pContext->cEventCommHead.m_u8EventType)
    {
        case 0x00:
            {
                DOLOG("[error]%s event_type:0, unknow event", __FUNCTION__);
                return -1;
            }
        case 0x04:
            {
                ParseRotateEvent();
                break;
            }

        case 0x0f:
            {
                ParseFormatEvent();
                break;
            }

        case 0x13:
            {
                ParseTablemapEvent();
                break;
            }
        case 0x17:
        case 0x18:
        case 0x19:
        case 0x1E:
        case 0x1F:
        case 0x20:
            {
                ParseRowEvent();
                break;
            }
        default:
            {
                //DOLOG("[trace]%s no process eventType. eventType:0x%X", __FUNCTION__, m_pContext->cEventCommHead.m_u8EventType);
                break;
            }
    }
    return 0;
}

int NormalPacket::ParseRotateEvent()
{
    int nRet = 0;

    uint32_t uBodyLenWithoutCrc = m_uBodyLen;
    if (m_pContext->bChecksumEnable == true && m_pContext->cFormatEvent.m_u8Alg != BINLOG_CHECKSUM_ALG_OFF && m_pContext->cFormatEvent.m_u8Alg != BINLOG_CHECKSUM_ALG_UNDEF)
        uBodyLenWithoutCrc -= BINLOG_CHECKSUM_LEN;

    nRet = m_pContext->cRotateEvent.Parse(m_pBodyBuf, m_uPos, uBodyLenWithoutCrc);

    m_pContext->sNextBinlogFileName = m_pContext->cRotateEvent.m_sNextBinlogFilename;
    m_pContext->uNextBinlogPos = m_pContext->cRotateEvent.m_uNextBinlogPos;

    DOLOG("[trace]%s rotate event, nextBinlogFileName:%s, nextBinlogPos:%u", __FUNCTION__, m_pContext->sNextBinlogFileName.c_str(), m_pContext->uNextBinlogPos);
    if (m_pContext->pIBusiness != NULL)
    {
        m_pContext->pIBusiness->SaveNextReqPos(m_pContext->sNextBinlogFileName, m_pContext->uNextBinlogPos);
    }


    return nRet;
}

int NormalPacket::ParseFormatEvent()
{
    int nRet = 0;
    //DOLOG("[trace]%s format event", __FUNCTION__);

    //一个binlog文件只有一个formatevent，一个新的 formatevent 到来，表明一个binlog文件解析完了，文件中的tablemapevent 自然就作废了。
    map<uint64_t, TablemapEvent *>::iterator it = m_pContext->mTidToTmEvent.begin();
    while (it != m_pContext->mTidToTmEvent.end())
    {
        if (it->second != NULL)
            delete it->second;
        m_pContext->mTidToTmEvent.erase(it++);
    }

    nRet = m_pContext->cFormatEvent.Parse(m_pBodyBuf, m_uPos, m_uBodyLen, m_pContext->bChecksumEnable);
    return nRet;
}

int NormalPacket::ParseTablemapEvent()
{
    int nRet = 0;
    //DOLOG("[trace]%s tablemap event", __FUNCTION__);

    uint32_t uBodyLenWithoutCrc = m_uBodyLen;
    if (m_pContext->bChecksumEnable == true && m_pContext->cFormatEvent.m_u8Alg != BINLOG_CHECKSUM_ALG_OFF && m_pContext->cFormatEvent.m_u8Alg != BINLOG_CHECKSUM_ALG_UNDEF)
        uBodyLenWithoutCrc -= BINLOG_CHECKSUM_LEN;

    uint64_t u64Tableid = TablemapEvent::GetTableid(m_pBodyBuf, m_uPos, &m_pContext->cFormatEvent);
    TablemapEvent *pTablemapEvent  = NULL;
    if (m_pContext->mTidToTmEvent.find(u64Tableid) == m_pContext->mTidToTmEvent.end())
    {
        pTablemapEvent = new TablemapEvent();
        if (pTablemapEvent == NULL)
        {
            DOLOG("[error]%s new TablemapEvent error.", __FUNCTION__);
            return -1;
        }
        m_pContext->mTidToTmEvent[u64Tableid] = pTablemapEvent;
    }
    else
    {
        pTablemapEvent = m_pContext->mTidToTmEvent[u64Tableid];
    }

    nRet = pTablemapEvent->Parse(m_pBodyBuf, m_uPos, uBodyLenWithoutCrc, &m_pContext->cFormatEvent);

    return nRet;
}

int NormalPacket::ParseRowEvent()
{
    int nRet = 0;
    //DOLOG("[trace]%s row event, eventType:0x%X", __FUNCTION__, m_pContext->cEventCommHead.m_u8EventType);

    uint32_t uBodyLenWithoutCrc = m_uBodyLen;
    if (m_pContext->bChecksumEnable == true && m_pContext->cFormatEvent.m_u8Alg != BINLOG_CHECKSUM_ALG_OFF && m_pContext->cFormatEvent.m_u8Alg != BINLOG_CHECKSUM_ALG_UNDEF)
        uBodyLenWithoutCrc -= BINLOG_CHECKSUM_LEN;

    uint64_t u64Tableid = RowEvent::GetTableid(m_pBodyBuf, m_uPos, m_pContext->cEventCommHead.m_u8EventType, &m_pContext->cFormatEvent);
    if (m_pContext->mTidToTmEvent.find(u64Tableid) == m_pContext->mTidToTmEvent.end())
    {
        DOLOG("[error]%s rowevent no found the right tabmapevent. tableid:%lu", __FUNCTION__, u64Tableid);
        return -1;
    }
    TablemapEvent *pTablemapEvent = m_pContext->mTidToTmEvent[u64Tableid];
    if (g_pConfig->GetMatchSchema(pTablemapEvent->m_sDatabaseName, pTablemapEvent->m_sTableName) == NULL)
    {
        //DOLOG("[trace]%s no match schema, databasename:%s, tablename:%s", __FUNCTION__, pTablemapEvent->m_sDatabaseName.c_str(), pTablemapEvent->m_sTableName.c_str());
        return 0;
    }

    if (m_pContext->pIBusiness != NULL)
    {
        m_pContext->pIBusiness->SaveNextReqPos(m_pContext->sNextBinlogFileName, m_pContext->uNextBinlogPos);
    }

    m_pContext->cRowEvent.Init(m_pContext->cEventCommHead.m_u8EventType);
    nRet = m_pContext->cRowEvent.Parse(m_pBodyBuf, m_uPos, uBodyLenWithoutCrc, &m_pContext->cFormatEvent, pTablemapEvent, m_pContext);

    return nRet;
}


/***********************class Packet*******************/
Packet::Packet()
{
    m_pContext = NULL;

    m_mapFlagToPacket.clear();

    memset(m_szHeadBuf, 0, sizeof(m_szHeadBuf));
    m_uBodyBufLen = 0;
    m_pBodyBuf = NULL;

    m_uHeadLen = 0;
    m_uBodyLen = 0;


}

Packet::~Packet()
{
    map<uint8_t, PacketBase *>::iterator it;
    for (it = m_mapFlagToPacket.begin(); it != m_mapFlagToPacket.end(); it++)
    {
        if (it->second != NULL)
        {
            delete it->second;
            it->second = NULL;
        }
    }

    if (m_pContext != NULL)
    {
        delete m_pContext;
        m_pContext = NULL;
    }
}

int Packet::Init(bool bChecksumEnable, IBusiness *pIBusiness)
{
    m_pContext = new tagContext();
    if (m_pContext == NULL)
        return -1;

    m_pContext->pIBusiness = pIBusiness;
    m_pContext->bChecksumEnable = bChecksumEnable;

    m_mapFlagToPacket.clear();
    ErrorPacket *pErrorPacket = new ErrorPacket();
    if (pErrorPacket == NULL)
        return -1;

    NormalPacket *pNormalPacket = new NormalPacket(m_pContext);
    if (pNormalPacket == NULL)
        return -1;

    m_mapFlagToPacket[0xff] = (PacketBase *)pErrorPacket;
    m_mapFlagToPacket[0x00] = (PacketBase *)pNormalPacket;

    memset(m_szHeadBuf, 0, sizeof(m_szHeadBuf));
    m_uBodyBufLen = 4096;
    m_pBodyBuf = (char *)malloc(m_uBodyBufLen);
    if (m_pBodyBuf == NULL)
        return -1;

    m_uHeadLen = 0;
    m_uBodyLen = 0;

    return 0;
}

int Packet::ReallocBuf(uint32_t uSize)
{
    if (uSize <= m_uBodyBufLen)
        return 0;

    char *pTemp = (char *)realloc(m_pBodyBuf, uSize + uSize/2);
    if (pTemp == NULL)
        DOLOG("[error]%s realloc error.", __FUNCTION__);

    m_pBodyBuf = pTemp;
    m_uBodyBufLen = uSize;

    return 0;
}

int Packet::ResetContext()
{
    if (m_pContext)
    {
        m_pContext->cFormatEvent.m_u8Alg = BINLOG_CHECKSUM_ALG_OFF;
        m_pContext->cFormatEvent.m_u8Alg = BINLOG_CHECKSUM_ALG_UNDEF;
    }
    return 0;
}

int Packet::Read(int fd)
{
    int nRet = 0;
    int nRead = 0;

    while (nRead != 4)
    {
        nRet = read(fd, m_szHeadBuf + nRead, 4 - nRead);
        if (nRet == -1)
        {
            return -1;
        }
        else if (nRet == 0)
        {
            return 0;
        }
        else 
        {
            nRead += nRet;
        }
    }

    m_uBodyLen = uint3korr(m_szHeadBuf);
    if (m_uBodyLen > m_uBodyBufLen)
        ReallocBuf(m_uBodyLen + m_uHeadLen);

    nRead = 0;
    while (nRead != (int)m_uBodyLen)
    {
        nRet = read(fd, m_pBodyBuf + nRead, m_uBodyLen - nRead);
        if (nRet == -1)
        {
            return -1;
        }
        else if (nRet == 0)
        {
            return 0;
        }
        else
        {
            nRead += nRet;
        }
    }

    return m_uHeadLen + m_uBodyLen;
}

int Packet::Parse()
{
    uint8_t u8ErrFlag = (uint8_t )m_pBodyBuf[0];
    if (u8ErrFlag != 0x00 && u8ErrFlag != 0xff)
        return -1;

    uint8_t u8Seq = (uint8_t) m_szHeadBuf[3];
    m_mapFlagToPacket[ u8ErrFlag ]->Init(m_uBodyLen, u8Seq, u8ErrFlag, m_pBodyBuf);
    m_mapFlagToPacket[ u8ErrFlag ]->Parse();

    if (u8ErrFlag == 0xff)
        return -1;
    return 0;
}

