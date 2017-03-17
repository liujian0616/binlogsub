#include "event.h"

/***********************class EventCommHead*******************/
int EventCommHead::Parse(char *pBuf, uint32_t &uPos)
{
    m_uTimestamp = uint4korr(pBuf + uPos);
    uPos += 4;
    m_u8EventType = pBuf[uPos];
    uPos += 1;
    m_uServerid = uint4korr(pBuf + uPos);
    uPos += 4;
    m_uEventSize = uint4korr(pBuf + uPos);
    uPos += 4;
    m_uLogPos = uint4korr(pBuf + uPos);
    uPos += 4;
    m_u16Flags = uint2korr(pBuf + uPos);
    uPos += 2;

    return 0;
}



/***********************class RotateEvent*******************/
int RotateEvent::Parse(char *pBuf, uint32_t &uPos, uint32_t uBufLen)
{
    m_uNextBinlogPos = uint8korr(pBuf + uPos);
    uPos += 8;

    int uLeft = uBufLen - uPos;
    m_sNextBinlogFilename.assign(pBuf+uPos, uLeft);;
    uPos += uLeft;

    return 0;
}



/***********************class FormatEvent*******************/
int FormatEvent::Parse(char *pBuf, uint32_t &uPos, uint32_t uBufLen, bool bChecksumEnable)
{
    m_u16BinlogVer = uint2korr(pBuf + uPos);
    uPos += 2;

    memset(m_szServerVer, 0, sizeof(m_szServerVer));
    strncpy(m_szServerVer, pBuf + uPos, 50);
    uPos += 50;

    m_uTimestamp = uint4korr(pBuf + uPos);
    uPos += 4;

    m_u8HeadLen = pBuf[uPos];
    uPos += 1;

    uint32_t  uPostHeadLenStrLen = 0;
    if (bChecksumEnable == true)
    {
        uPostHeadLenStrLen  = uBufLen - uPos - BINLOG_CHECKSUM_LEN - BINLOG_CHECKSUM_ALG_DESC_LEN;
        m_sPostHeadLenStr.assign(pBuf + uPos, uPostHeadLenStrLen);
        uPos += uPostHeadLenStrLen;

        m_u8Alg = pBuf[uPos];
        uPos += 1;
    }
    else
    {
        uPostHeadLenStrLen = uBufLen - uPos;
        m_sPostHeadLenStr.assign(pBuf + uPos, uPostHeadLenStrLen);
    }

    return 0;
}

uint8_t FormatEvent::GetPostHeadLen(uint32_t uType)
{
    assert(uType <= m_sPostHeadLenStr.size());
    return m_sPostHeadLenStr[uType - 1];
}



/***********************class TablemapEvent*******************/
uint64_t TablemapEvent::GetTableid(char *pBuf, uint32_t uPos, FormatEvent *pFormatEvent)
{
    uint64_t u64Tableid = 0;
    uint8_t u8PostHeadLen = pFormatEvent->GetPostHeadLen(0x13);

    //DOLOG("[trace]TablemapEvent::GetTableid, u8PostHeadLen:%d", u8PostHeadLen);
    //postheader 部分,tableid占 6或8字节 
    if (u8PostHeadLen == 6) 
    {
        u64Tableid = uint4korr(pBuf + uPos);
        uPos += 4;
    }
    else
    {
        u64Tableid = uint6korr(pBuf + uPos);
        uPos += 6;
    }

    return u64Tableid;
}

int TablemapEvent::Parse(char *pBuf, uint32_t &uPos, uint32_t uBufLen, FormatEvent *pFormatEvent)
{
    uint8_t u8PostHeadLen = pFormatEvent->GetPostHeadLen(m_u8Cmd);
    //postheader 部分,tableid占 6或8字节 
    if (u8PostHeadLen == 6)
    {
        m_u64Tableid = uint4korr(pBuf + uPos);
        uPos += 4;
    }
    else
    {
        m_u64Tableid = uint6korr(pBuf + uPos);
        uPos += 6;
    }

    m_u16Flags = uint2korr(pBuf + uPos);
    uPos += 2;

    //payload 部分
    m_u8DatabaseNameLength = pBuf[uPos];
    uPos += 1;
    m_sDatabaseName.assign(pBuf + uPos, m_u8DatabaseNameLength);
    uPos += m_u8DatabaseNameLength;
    uPos += 1;

    m_u8TableNameLength = pBuf[uPos];
    uPos += 1;
    m_sTableName.assign(pBuf + uPos, m_u8TableNameLength);
    uPos += m_u8TableNameLength;
    uPos += 1;

    UnpackLenencInt(pBuf + uPos, uPos, m_u64ColumnCount);

    m_cColumnTypeDef.Assign(pBuf + uPos, m_u64ColumnCount);
    uPos += m_u64ColumnCount;

    UnpackLenencInt(pBuf + uPos, uPos, m_u64ColumnMetaLength);
    m_cColumnMetaDef.Assign(pBuf + uPos, m_u64ColumnMetaLength);
    uPos += m_u64ColumnMetaLength;

    uint64_t u64NullBitmapLength = (m_u64ColumnCount + 7) / 8;
    m_cNullBit.Assign(pBuf + uPos, u64NullBitmapLength);
    uPos += u64NullBitmapLength;


    //根据每一列的类型解析 metadata 中的数据, 存入m_vMetaVec;
    uint32_t uMetaPos = 0;
    uint16_t uMeta = 0;
    for (uint32_t i = 0; i < m_u64ColumnCount; i++)
    {
        uMeta = ReadMeta(m_cColumnMetaDef, uMetaPos, m_cColumnTypeDef.At(i));
        m_vMeta.push_back(uMeta);
    }
    return 0;
}

uint16_t TablemapEvent::ReadMeta(MemBlock &cColumnMetaDef, uint32_t &uMetaPos, uint8_t u8ColumnType)
{
    uint16_t u16Meta = 0;
    if (u8ColumnType == MYSQL_TYPE_TINY_BLOB ||
            u8ColumnType == MYSQL_TYPE_BLOB ||
            u8ColumnType == MYSQL_TYPE_MEDIUM_BLOB ||
            u8ColumnType == MYSQL_TYPE_LONG_BLOB ||
            u8ColumnType == MYSQL_TYPE_DOUBLE ||
            u8ColumnType == MYSQL_TYPE_FLOAT ||
            u8ColumnType == MYSQL_TYPE_GEOMETRY ||
            u8ColumnType == MYSQL_TYPE_TIME2 ||
            u8ColumnType == MYSQL_TYPE_DATETIME2 ||
            u8ColumnType == MYSQL_TYPE_TIMESTAMP2)
    {
        u16Meta = (uint16_t)(uint8_t)cColumnMetaDef.At(uMetaPos);
        uMetaPos += 1;
    }
    else if(u8ColumnType == MYSQL_TYPE_NEWDECIMAL ||
            u8ColumnType == MYSQL_TYPE_VAR_STRING ||
            u8ColumnType == MYSQL_TYPE_STRING ||
            u8ColumnType == MYSQL_TYPE_SET ||
            u8ColumnType == MYSQL_TYPE_ENUM) 
    {
        u16Meta = ((uint16_t)(uint8_t)cColumnMetaDef.At(uMetaPos) << 8) | ((uint16_t)(uint8_t)cColumnMetaDef.At(uMetaPos + 1));
        uMetaPos += 2;
    }
    else if (u8ColumnType == MYSQL_TYPE_VARCHAR) 
    {
        u16Meta = uint2korr(cColumnMetaDef.m_pBlock + uMetaPos);
        uMetaPos += 2;
    }
    else if (u8ColumnType == MYSQL_TYPE_BIT) 
    {
        /*
         *@todo 第一字节 length%8 第二字节 length/8
         */
        u16Meta = ((uint16_t)(uint8_t)cColumnMetaDef.At(uMetaPos) << 8) | ((uint16_t)(uint8_t)cColumnMetaDef.At(uMetaPos + 1));
        uMetaPos += 2;

    }
    else if (u8ColumnType == MYSQL_TYPE_DECIMAL ||
            u8ColumnType == MYSQL_TYPE_TINY ||
            u8ColumnType == MYSQL_TYPE_SHORT ||
            u8ColumnType == MYSQL_TYPE_LONG ||
            u8ColumnType == MYSQL_TYPE_NULL ||
            u8ColumnType == MYSQL_TYPE_TIMESTAMP ||
            u8ColumnType == MYSQL_TYPE_LONGLONG ||
            u8ColumnType == MYSQL_TYPE_INT24 ||
            u8ColumnType == MYSQL_TYPE_DATE ||
            u8ColumnType == MYSQL_TYPE_TIME ||
            u8ColumnType == MYSQL_TYPE_DATETIME ||
            u8ColumnType == MYSQL_TYPE_YEAR) 
    {
        u16Meta = 0;
    }
    else 
    {
        DOLOG("[error]%s columntype can not exist in the binlog, columntype:%d", __FUNCTION__, u8ColumnType);
    }

    return u16Meta;
}


uint8_t TablemapEvent::GetColumnType(uint32_t uColumnIndex)
{
    return m_cColumnTypeDef.At(uColumnIndex);
}

uint16_t TablemapEvent::GetColumnMeta(uint32_t uColumnIndex)
{
    return m_vMeta[uColumnIndex];
}

/***********************class RowEvent*******************/
int RowEvent::Init(uint8_t u8Cmd)
{
    m_u8Cmd = u8Cmd;
    if (u8Cmd == 0x17 || u8Cmd == 0x18 || u8Cmd == 0x19)
        m_u8Ver = 1;
    else if (u8Cmd == 0x1E || u8Cmd == 0x1F || u8Cmd == 0x20)
        m_u8Ver = 2;

    return 0;
}

uint64_t RowEvent::GetTableid(char *pBuf, uint32_t uPos, uint8_t u8Cmd, FormatEvent *pFormatEvent)
{
    uint64_t u64Tableid = 0;
    uint8_t u8PostHeadLen = pFormatEvent->GetPostHeadLen(u8Cmd);

    //DOLOG("[trace]RowEvent::GetTableid, u8PostHeadLen:%d", u8PostHeadLen);
    //postheader 部分,tableid占 6或10字节 
    if (u8PostHeadLen == 6)
    {
        u64Tableid = uint4korr(pBuf + uPos);
        uPos += 4;
    }
    else 
    {
        u64Tableid = uint6korr(pBuf + uPos);
        uPos += 6;
    }

    return u64Tableid;
}

int RowEvent::Parse(char *pBuf, uint32_t &uPos, uint32_t uBufLen, FormatEvent *pFormatEvent, TablemapEvent *pTablemapEvent, tagContext *pContext)
{
    uint8_t u8PostHeadLen = pFormatEvent->GetPostHeadLen(m_u8Cmd);
    //postheader 部分,tableid占 6或10字节 
    if (u8PostHeadLen == 6)
    {
        m_u64Tableid = uint4korr(pBuf + uPos);
        uPos += 4;
    }
    else 
    {
        m_u64Tableid = uint6korr(pBuf + uPos);
        uPos += 6;
    }
    m_u16Flags = uint2korr(pBuf + uPos);
    uPos += 2;

    if (m_u8Ver == 2)
    {
        m_u16ExtraDataLength = uint2korr(pBuf + uPos);
        uPos += 2;
        uint16_t u16RealExtraDataLength = m_u16ExtraDataLength - 2;
        if (u16RealExtraDataLength > 0)
        {
            m_sExtraData.assign(pBuf + uPos, u16RealExtraDataLength);
            uPos += u16RealExtraDataLength;
        }
    }

    //payload 部分
    UnpackLenencInt(pBuf + uPos, uPos, m_u64ColumnCount);

    uint32_t uPresentBitmapLen = (m_u64ColumnCount + 7) / 8;
    m_cPresentBitmap1.Assign(pBuf + uPos, uPresentBitmapLen);
    uPos += uPresentBitmapLen;

    uint32_t uPresentBitmapBitset1 = m_cPresentBitmap1.GetBitsetCount(m_u64ColumnCount);
    uint32_t uNullBitmap1Size = (uPresentBitmapBitset1 + 7) / 8;

    uint32_t uNullBitmap2Size = 0; 
    if (m_u8Cmd == 0x18 || m_u8Cmd == 0x1F)
    {
        m_cPresentBitmap2.Assign(pBuf + uPos, uPresentBitmapLen);
        uPos += uPresentBitmapLen;

        uint32_t uPresentBitmapBitset2 = m_cPresentBitmap2.GetBitsetCount(m_u64ColumnCount);
        uNullBitmap2Size = (uPresentBitmapBitset2 + 7) / 8;
    }

    // rows 数据,可能有多行
    while (uBufLen > uPos)
    {
        Row *pRow = new Row();
        if (pRow == NULL)
        {
            DOLOG("[error]%s new Row() error", __FUNCTION__);
            return -1;
        }

        if (m_u8Cmd == 0x17 || m_u8Cmd == 0x1E)
        {
            pRow->Init(INSERT, pTablemapEvent->m_sDatabaseName, pTablemapEvent->m_sTableName);
            ParseRow(pBuf, uPos, uNullBitmap1Size, pRow, &m_cPresentBitmap1, false, pTablemapEvent);
        }
        else if (m_u8Cmd == 0x19 || m_u8Cmd == 0x20)
        {
            pRow->Init(DEL, pTablemapEvent->m_sDatabaseName, pTablemapEvent->m_sTableName);
            ParseRow(pBuf, uPos, uNullBitmap1Size, pRow, &m_cPresentBitmap1, false, pTablemapEvent);
        }
        else if (m_u8Cmd == 0x18 || m_u8Cmd == 0x1F)
        {
            pRow->Init(UPDATE, pTablemapEvent->m_sDatabaseName, pTablemapEvent->m_sTableName);

            ParseRow(pBuf, uPos, uNullBitmap1Size, pRow, &m_cPresentBitmap1, true, pTablemapEvent);
            ParseRow(pBuf, uPos, uNullBitmap2Size, pRow, &m_cPresentBitmap2, false, pTablemapEvent);
        }
        else
        {
            DOLOG("[error]%s unsport commond=%hhd", __FUNCTION__, m_u8Cmd);
            return -1;
        }

        //pRow->Print();

        Row *pFilteredRow = new Row();
        if (pFilteredRow == NULL)
        {
            DOLOG("[error]%s new Row() error", __FUNCTION__);
            return -1;
        }
        pFilteredRow->FilterCopy(pRow);


        //pFilteredRow->Print();
        if (pContext->pIBusiness != NULL)
        {
            pContext->pIBusiness->IncrProcess(pFilteredRow);
        }

        delete pRow;
        delete pFilteredRow;
    }

    return 0;
}

int RowEvent::ParseRow(char *pBuf, uint32_t &uPos, uint32_t uNullBitmapSize, Row *pRow, MemBlock *pPresentBitmap, bool bOld, TablemapEvent *pTablemapEvent)
{
    int nRet = 0;
    bool bUnsigned = false;

    m_cNullBitmap.Assign(pBuf + uPos, uNullBitmapSize);
    uPos += uNullBitmapSize;

    uint32_t uPresentPos = 0;
    for (uint32_t i = 0; i < m_u64ColumnCount; i++)
    {
        if (pPresentBitmap->GetBit(i))
        {
            bool bNullCol = m_cNullBitmap.GetBit(uPresentPos);
            if (bNullCol)
            {
                pRow->PushBack(string(""), bOld);
            }
            else
            {
                uint8_t u8ColType = pTablemapEvent->GetColumnType(i);
                uint16_t u16Meta = pTablemapEvent->GetColumnMeta(i);
                nRet = ParseColumnValue(pBuf + uPos, uPos, u8ColType, u16Meta, bOld, bUnsigned, pRow);
                if (nRet == -1)
                    return -1;

            }

            uPresentPos++;
        }
        else
        {
            pRow->PushBack(string(""), bOld);
        }
        
    }

    return 0;
}

int RowEvent::ParseColumnValue(char *pBuf, uint32_t &uPos, uint8_t u8ColType, uint16_t u16Meta, bool bOld, bool bUnsigned, Row *pRow)
{
    uint32_t uLength = 0;
    //由于mysql的历史原因，SET,ENUM,STRING 的coltype 都是 MYSQL_TYPE_STRING，需要用meta去进行进一步确认具体的类型
    if (u8ColType == MYSQL_TYPE_STRING)
    {
        if (u16Meta >= 256)
        {
            uint8_t u8Byte0 = u16Meta >> 8;
            uint8_t u8Byte1 = u16Meta & 0xFF;
            if ((u8Byte0 & 0x30) != 0x30)
            {
                //a long CHAR() field: see #37426 https://bugs.mysql.com/bug.php?id=37426
                uLength= u8Byte1 | (((u8Byte0 & 0x30) ^ 0x30) << 4);
                u8ColType= u8Byte0 | 0x30;
            }
            else
            {
                switch (u8Byte0)
                {
                    case MYSQL_TYPE_SET:
                    case MYSQL_TYPE_ENUM:
                    case MYSQL_TYPE_STRING:
                        u8ColType = u8Byte0;
                        uLength = u8Byte1;
                        break;
                    default:
                        DOLOG("don't know how to handle column type=%d meta=%d, byte0=%d, byte1=%d", u8ColType, u16Meta, u8Byte0, u8Byte1);
                        return -1;
                }
            }
        }
        else
        {
            uLength = u16Meta;
        }
    }

    char szValue[256] = {0};
    switch (u8ColType)
    {
        case MYSQL_TYPE_LONG://int32
            {
                if (bUnsigned) 
                {
                    uint32_t i = uint4korr(pBuf);
                    snprintf(szValue, sizeof(szValue), "%u", i);
                } 
                else 
                {
                    int32_t i = sint4korr(pBuf);
                    snprintf(szValue, sizeof(szValue), "%d", i);
                }
                uPos += 4;
                pRow->PushBack(szValue, bOld);
                break;
            }
        case MYSQL_TYPE_TINY:
            {
                if (bUnsigned)
                {
                    uint8_t c = (uint8_t) (*pBuf);
                    snprintf(szValue, sizeof(szValue), "%hhu", c);
                }
                else
                {
                    int8_t c = (int8_t) (*pBuf);
                    snprintf(szValue, sizeof(szValue), "%hhd", c);
                }
                uPos += 1;
                pRow->PushBack(szValue, bOld);
                break;
            }
        case MYSQL_TYPE_SHORT:
            {
                if (bUnsigned)
                {
                    uint16_t s = uint2korr(pBuf);
                    snprintf(szValue, sizeof(szValue), "%hu", s);
                }
                else
                {
                    int16_t s = sint2korr(pBuf);
                    snprintf(szValue, sizeof(szValue), "%hd", s);
                }
                uPos += 2;
                pRow->PushBack(szValue, bOld);
                break;
            }
        case MYSQL_TYPE_LONGLONG:
            {
                if (bUnsigned)
                {
                    uint64_t ll = uint8korr(pBuf);
                    snprintf(szValue, sizeof(szValue), "%lu", ll);
                }
                else
                {
                    int64_t ll = sint8korr(pBuf);
                    snprintf(szValue, sizeof(szValue), "%ld", ll);
                }
                uPos += 8;
                pRow->PushBack(szValue, bOld);
                break;
            }
        case MYSQL_TYPE_INT24:
            {
                int32_t i;
                if (bUnsigned)
                {
                    i = uint3korr(pBuf);
                }
                else
                {
                    i = sint3korr(pBuf);
                }
                snprintf(szValue, sizeof(szValue), "%d", i);
                uPos += 3;
                pRow->PushBack(szValue, bOld);
                break;
            }
        case MYSQL_TYPE_TIMESTAMP:
            {
                uint32_t i = uint4korr(pBuf);
                snprintf(szValue, sizeof(szValue), "%u", i);
                uPos += 4;
                pRow->PushBack(szValue, bOld);
                break;
            }
        case MYSQL_TYPE_DATETIME:
            {
                uint64_t ll = uint8korr(pBuf);
                uint32_t d = ll / 1000000;
                uint32_t t = ll % 1000000;
                snprintf(szValue, sizeof(szValue),
                        "%04d-%02d-%02d %02d:%02d:%02d",
                        d / 10000, (d % 10000) / 100, d / 100,
                        t / 10000, (t % 10000) / 100, t / 100);
                uPos += 8;
                pRow->PushBack(szValue, bOld);
                break;
            }
        case MYSQL_TYPE_TIME:
            {
                uint32_t i32 = uint3korr(pBuf);
                snprintf(szValue, sizeof(szValue), "%02d:%02d:%02d", i32 / 10000, (i32 % 10000) / 100, i32 / 100);
                uPos += 3;
                pRow->PushBack(szValue, bOld);
                break;
            }
        case MYSQL_TYPE_NEWDATE:
            {
                uint32_t tmp= uint3korr(pBuf);
                int part;
                char sbuf[11];
                char *spos= &sbuf[10];  // start from '\0' to the beginning

                /* Copied from field.cc */
                *spos--=0;                 // End NULL
                part=(int) (tmp & 31);
                *spos--= (char) ('0'+part%10);
                *spos--= (char) ('0'+part/10);
                *spos--= ':';
                part=(int) (tmp >> 5 & 15);
                *spos--= (char) ('0'+part%10);
                *spos--= (char) ('0'+part/10);
                *spos--= ':';
                part=(int) (tmp >> 9);
                *spos--= (char) ('0'+part%10); part/=10;
                *spos--= (char) ('0'+part%10); part/=10;
                *spos--= (char) ('0'+part%10); part/=10;
                *spos= (char) ('0'+part);

                snprintf(szValue, sizeof(szValue), "%s", sbuf);
                uPos += 3;
                pRow->PushBack(szValue, bOld);
                break;
            }
        case MYSQL_TYPE_DATE:
            {
                uint32_t i32 = uint3korr(pBuf);
                snprintf(szValue, sizeof(szValue), "%04d-%02d-%02d", (int32_t)(i32 / (16L * 32L)), (int32_t)(i32 / 32L % 16L), (int32_t)(i32 % 32L));
                uPos += 3;
                pRow->PushBack(szValue, bOld);
                break;
            }
        case MYSQL_TYPE_YEAR:
            {
                uint32_t i = (uint32_t)(uint8_t)*pBuf;
                snprintf(szValue, sizeof(szValue), "%04d", i + 1900);
                uPos += 1;
                pRow->PushBack(szValue, bOld);
                break;
            }
        case MYSQL_TYPE_ENUM:
            {
                switch (uLength) {
                    case 1:
                        {
                            snprintf(szValue, sizeof(szValue), "%d", (int32_t)*pBuf);
                            uPos += 1;
                            pRow->PushBack(szValue, bOld);
                            break;
                        }
                    case 2:
                        {
                            int32_t i32 = uint2korr(pBuf);
                            snprintf(szValue, sizeof(szValue) ,"%d", i32);
                            uPos += 2;
                            pRow->PushBack(szValue, bOld);
                            break;
                        }
                    default:
                        DOLOG("[error]%s unknown enum packlen=%d", __FUNCTION__, uLength);
                        return -1;
                }
                break;
            }
        case MYSQL_TYPE_SET:
            {
                uPos += uLength;
                pRow->PushBack(pBuf, uLength, bOld);
                break;
            }
        case MYSQL_TYPE_BLOB:
            {
                switch (u16Meta)
                {
                    case 1:     //TINYBLOB/TINYTEXT
                        {
                            uLength = (uint8_t)(*pBuf);
                            uPos += uLength + 1;
                            pRow->PushBack(pBuf + 1, uLength, bOld);
                            break;
                        }
                    case 2:     //BLOB/TEXT
                        {
                            uLength = uint2korr(pBuf);
                            uPos += uLength + 2;
                            pRow->PushBack(pBuf + 2, uLength, bOld);
                            break;
                        }
                    case 3:     //MEDIUMBLOB/MEDIUMTEXT
                        {
                            uLength = uint3korr(pBuf);
                            uPos += uLength + 3;
                            pRow->PushBack(pBuf + 3, uLength, bOld);
                            break;
                        }
                    case 4:     //LONGBLOB/LONGTEXT
                        {
                            uLength = uint4korr(pBuf);
                            uPos += uLength + 4;
                            pRow->PushBack(pBuf + 4, uLength, bOld);
                            break;
                        }
                    default:
                        DOLOG("[error]%s unknown blob type=%d", __FUNCTION__, u16Meta);
                        return -1;
                }
                break;
            }
        case MYSQL_TYPE_VARCHAR:
        case MYSQL_TYPE_VAR_STRING:
            {
                uLength = u16Meta;
                if (uLength < 256){
                    uLength = (uint8_t)(*pBuf);
                    uPos += 1 + uLength;
                    pRow->PushBack(pBuf + 1, uLength, bOld);
                } else {
                    uLength = uint2korr(pBuf);
                    uPos += 2 + uLength;
                    pRow->PushBack(pBuf + 2, uLength, bOld);
                }
                break;
            }
        case MYSQL_TYPE_STRING:
            {
                if (uLength < 256){
                    uLength = (uint8_t)(*pBuf);
                    uPos += 1 + uLength;
                    pRow->PushBack(pBuf + 1, uLength, bOld);
                }else{
                    uLength = uint2korr(pBuf);
                    uPos += 2 + uLength;
                    pRow->PushBack(pBuf + 2, uLength, bOld);
                }
                break;
            }
        case MYSQL_TYPE_BIT:
            {
                uint32_t nBits= (u16Meta >> 8)  + (u16Meta & 0xFF) * 8;
                uLength= (nBits + 7) / 8;
                uPos += uLength;
                pRow->PushBack(pBuf, uLength, bOld);
                break;
            }
        case MYSQL_TYPE_FLOAT:
            {
                float fl;
                float4get(fl, pBuf);
                uPos += 4;
                snprintf(szValue, sizeof(szValue), "%f", (double)fl);
                pRow->PushBack(szValue, bOld);
                break;
            }
        case MYSQL_TYPE_DOUBLE:
            {
                double dbl;
                float8get(dbl,pBuf);
                uPos += 8;
                snprintf(szValue, sizeof(szValue), "%f", dbl);
                pRow->PushBack(szValue, bOld);
                break;
            }
        case MYSQL_TYPE_NEWDECIMAL:
            {
                uint8_t precision= u16Meta >> 8;
                uint8_t decimals= u16Meta & 0xFF;
                int bin_size = my_decimal_get_binary_size(precision, decimals);
                my_decimal dec;
                binary2my_decimal(E_DEC_FATAL_ERROR, (unsigned char*) pBuf, &dec, precision, decimals);
                int i, end;
                char buff[512], *spos;
                spos = buff;
                spos += sprintf(buff, "%s", dec.sign() ? "-" : "");
                end= ROUND_UP(dec.frac) + ROUND_UP(dec.intg)-1;
                for (i=0; i < end; i++)
                    spos += sprintf(spos, "%09d.", dec.buf[i]);
                spos += sprintf(spos, "%09d", dec.buf[i]);
                pRow->PushBack(buff, bOld);
                uPos += bin_size;
                break;
            }
        case MYSQL_TYPE_TIMESTAMP2: //mysql 5.6新增类型
            {
                struct timeval tm;
                my_timestamp_from_binary(&tm, pBuf, uPos,  u16Meta);
                snprintf(szValue, sizeof(szValue), "%u.%u", (uint32_t)tm.tv_sec, (uint32_t)tm.tv_usec);
                pRow->PushBack(szValue, bOld);
                break;
            }
        case MYSQL_TYPE_DATETIME2:  //mysql 5.6新增类型
            {
                int64_t ymd, hms;
                int64_t ymdhms, ym;
                int frac;
                int64_t packed= my_datetime_packed_from_binary(pBuf, uPos, u16Meta);
                if (packed < 0)
                    packed = -packed;

                ymdhms= MY_PACKED_TIME_GET_INT_PART(packed);
                frac = MY_PACKED_TIME_GET_FRAC_PART(packed);

                ymd= ymdhms >> 17;
                ym= ymd >> 5;
                hms= ymdhms % (1 << 17);

                int day= ymd % (1 << 5);
                int month= ym % 13;
                int year= ym / 13;

                int second= hms % (1 << 6);
                int minute= (hms >> 6) % (1 << 6);
                int hour= (hms >> 12);

                snprintf(szValue, sizeof(szValue), "%04d-%02d-%02d %02d:%02d:%02d.%d", year, month, day, hour, minute, second, frac);
                pRow->PushBack(szValue, bOld);
                break;
            }
        case MYSQL_TYPE_TIME2: //mysql 5.6新增类型
            {
                assert(u16Meta <= DATETIME_MAX_DECIMALS);
                int64_t packed= my_time_packed_from_binary(pBuf, uPos, u16Meta);
                if (packed < 0)
                    packed = -packed;

                long hms = MY_PACKED_TIME_GET_INT_PART(packed);
                int frac = MY_PACKED_TIME_GET_FRAC_PART(packed);

                int hour= (hms >> 12) % (1 << 10);
                int minute= (hms >> 6)  % (1 << 6);
                int second= hms        % (1 << 6);
                snprintf(szValue, sizeof(szValue), "%02d:%02d:%02d.%d", hour, minute, second, frac);
                pRow->PushBack(szValue, bOld);
                break;
            }

        default:
            DOLOG("[error]%s don't know how to handle type =%d, meta=%d column value", __FUNCTION__, u8ColType, u16Meta);
            return -1;

    }

    return 0;
}
