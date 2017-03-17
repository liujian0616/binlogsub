#ifndef EVENT_H
#define EVENT_H

#include <string>
#include <vector>
#include <map>
#include "memblock.h"
#include "myconvert.h"
#include "commonDefine.h"
#include "util.h"
#include "log.h"
#include "row.h"
#include "my_time.h"
#include "mydecimal.h"
#include "config.h"
#include "business.h"

using std::string;
using std::vector;
using std::map;

class EventCommHead;
class RotateEvent;
class FormatEvent;
class TablemapEvent;
class RowEvent;


class EventCommHead
{

    public:
        EventCommHead(){};
        ~EventCommHead(){};

        int Parse(char *pBuf, uint32_t &uPos);

    public:
        uint32_t m_uTimestamp;
        uint8_t  m_u8EventType;
        uint32_t m_uServerid;
        uint32_t m_uEventSize;
        uint32_t m_uLogPos;
        uint16_t m_u16Flags;
};

class RotateEvent
{

    public:
        RotateEvent(){};
        ~RotateEvent(){};

        int Parse(char *pBuf, uint32_t &uPos, uint32_t uBufLen);

    public:
        uint32_t m_uNextBinlogPos;
        string  m_sNextBinlogFilename;
};

class FormatEvent
{
    public:
        FormatEvent() : m_u8Alg(0) {};
        ~FormatEvent(){};

        int Parse(char *pBuf, uint32_t &uPos, uint32_t uBufLen, bool bChecksumEnable);
        uint8_t GetPostHeadLen(uint32_t uType);

    public:
        uint16_t m_u16BinlogVer;
        char m_szServerVer[51];
        uint32_t m_uTimestamp;
        uint8_t m_u8HeadLen;
        string m_sPostHeadLenStr; 
        uint8_t m_u8Alg;
};

class TablemapEvent
{

    public:
        TablemapEvent() :m_u8Cmd(0x13) {};
        ~TablemapEvent(){};

        static uint64_t GetTableid(char *pBuf, uint32_t uPos, FormatEvent *pFormatEvent);
        int Parse(char *pBuf, uint32_t &uPos,  uint32_t uBufLen, FormatEvent *pFormatEvent);

        uint16_t ReadMeta(MemBlock &cColumnMetaDef, uint32_t &uMetaPos, uint8_t u8ColumnType);
        uint8_t GetColumnType(uint32_t uColumnIndex);
        uint16_t GetColumnMeta(uint32_t uColumnIndex);

    public:
        uint8_t m_u8Cmd;
        uint64_t m_u64Tableid;
        uint16_t m_u16Flags;
        uint8_t m_u8DatabaseNameLength;
        string m_sDatabaseName;
        uint8_t m_u8TableNameLength;
        string m_sTableName;
        uint64_t m_u64ColumnCount;
        MemBlock m_cColumnTypeDef;
        uint64_t m_u64ColumnMetaLength;
        MemBlock m_cColumnMetaDef;
        MemBlock m_cNullBit;

        vector<uint16_t> m_vMeta;     //解析后的meta存入这里
};

class RowEvent
{

    public:
        RowEvent(){};
        ~RowEvent(){};

        int Init(uint8_t u8Cmd);
        int Parse(char *pBuf, uint32_t &uPos, uint32_t uBufLen, FormatEvent *pFormatEvent, TablemapEvent *pTablemapEvent, tagContext *pContext);
        int ParseRow(char *pBuf, uint32_t &uPos, uint32_t uNullBitmapSize, Row *pRow, MemBlock *pPresentBitmap, bool bOld, TablemapEvent *pTablemapEvent);
        int ParseColumnValue(char *pBuf, uint32_t &uPos, uint8_t u8ColType, uint16_t u16Meta, bool bOld, bool bUnsigned, Row *pRow);

        static uint64_t GetTableid(char *pBuf, uint32_t uPos, uint8_t u8Cmd, FormatEvent *pFormatEvent);

    public:
        uint8_t m_u8Ver;
        uint8_t m_u8Cmd;
        uint64_t m_u64Tableid;
        uint16_t m_u16Flags;
        uint16_t m_u16ExtraDataLength;
        string  m_sExtraData;
        uint64_t m_u64ColumnCount;
        MemBlock m_cPresentBitmap1;
        MemBlock m_cPresentBitmap2;
        MemBlock m_cNullBitmap;
};

struct tagContext
{
    EventCommHead   cEventCommHead;
    RotateEvent     cRotateEvent;
    FormatEvent     cFormatEvent;
    map<uint64_t, TablemapEvent *> mTidToTmEvent;
    RowEvent        cRowEvent;
    IBusiness       *pIBusiness;
    string          sNextBinlogFileName;
    uint32_t        uNextBinlogPos;
};

#endif
