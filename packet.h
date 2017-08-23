#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>
#include <string.h>
#include <string>
#include <map>
#include "myconvert.h"
#include "log.h"
#include "event.h"
#include "config.h"
#include "business.h"
#include "commonDefine.h"
using std::string;
using std::map;

class NormalPacket;

#if 0
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



class PacketBase
{
    public:
        PacketBase() {}; 
        ~PacketBase() {};

        virtual int Init(uint32_t uBodyLen, uint8_t u8Seq, uint8_t u8ErrFlag, char *pBdoyBuf);
        virtual int Parse() = 0;

    protected:
        uint32_t    m_uBodyLen;
        uint8_t     m_u8Seq;
        uint8_t     m_u8ErrFlag;
        char        *m_pBodyBuf;    //记录body的起始位置,包含errflag
        uint32_t    m_uPos;         //记录解析到那个位置

};

class ErrorPacket : public PacketBase
{
    public:
        ErrorPacket() {};
        ~ErrorPacket() {};

        virtual int Parse();
        int PrintInfo();

    private:
        uint16_t    m_u16ErrCode;
        char        m_szState[6];
        string      m_sInfo;
};

/*
 *@todo 正常包的结构为 
 *    bodylen 3
 *    seq     1
 *    errflag 1
 *    eventcommhead  19
 *    eventbody      var
 * 真正的数据存在与rowevent中，解析rowevent需要 其他event进行辅助，所以本类存了其他的event 作为上下文
 */
class NormalPacket : public PacketBase
{
    public:
        NormalPacket(bool bChecksumEnable, tagContext *pContext) 
            : m_pContext(pContext) {};
        ~NormalPacket(){};

        virtual int Parse();

        int ParseRotateEvent();
        int ParseFormatEvent();
        int ParseTablemapEvent();
        int ParseRowEvent();

        tagContext      *m_pContext;
};


class Packet
{
    public:
        Packet();
        virtual ~Packet();

        int Init(bool bChecksumEnable, IBusiness *pIBusiness);
        int Read(int fd);
        int Parse();
        int ReallocBuf(uint32_t uSize);
        int ResetContext();

    private:
        map<uint8_t, PacketBase*>   m_mapFlagToPacket;
        char                        m_szHeadBuf[4];
        char                        *m_pBodyBuf;
        uint32_t                    m_uBodyBufLen;

        uint32_t                    m_uHeadLen;
        uint32_t                    m_uBodyLen;

        tagContext                  *m_pContext;
};

#endif
