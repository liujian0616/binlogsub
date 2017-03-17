#include "memblock.h"

MemBlock::MemBlock()
{
    m_uSize = 0;
    m_pBlock = NULL;
}

MemBlock::~MemBlock()
{
    if (m_pBlock != NULL)
    {
        delete m_pBlock;
        m_pBlock = NULL;
        m_uSize = 0;
    }
}


int MemBlock::Assign(char *pBuf, uint32_t uSize)
{
    if (m_pBlock != NULL)
    {
        delete m_pBlock;
        m_pBlock = NULL;
        m_uSize = 0;
    }

    m_pBlock = new char[uSize];
    if (m_pBlock == NULL)
        return -1;

    memcpy(m_pBlock, pBuf, uSize);
    m_uSize = uSize;

    return 0;
}

char MemBlock::At(uint32_t uIndex)
{
    assert(uIndex < m_uSize);
    return *(m_pBlock + uIndex);
}

//判断某个bit位是否为 1
bool MemBlock::GetBit(uint32_t uPos)
{
    uint32_t mchar = uPos / 8;  //字节序号
    uint32_t nbit = uPos & 0x07; //字节中的bit序号
    assert(mchar < m_uSize);
    return ((m_pBlock[mchar] >> nbit) & 0x01) == 0x01 ? true : false;
}

uint32_t MemBlock::GetBitsetCount()
{
    uint32_t uCount = 0;
    for (uint32_t i = 0; i < m_uSize; i++)
    {
        uint8_t p = *(m_pBlock + i);
        while (p != 0)
        {
            if ((p & 0x01) != 0)
                uCount++;
            p = p >> 1;
        }
    }

    return uCount;
}

//获取 0~uBitsetSize 范围内 bit位为 1 的位数
uint32_t MemBlock::GetBitsetCount(uint32_t uBitsetSize)
{
    uint32_t uCount = 0;
    assert(uBitsetSize <= m_uSize * 8);
    for (uint32_t i = 0; i < uBitsetSize; i++)
    {
        if (GetBit(i))
            uCount++;
    }

    return uCount;
}
