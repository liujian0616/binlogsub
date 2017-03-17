#ifndef MEMBLOCK_H
#define MEMBLOCK_H

#include <stdint.h>
#include <string.h>
#include <assert.h>

class MemBlock
{
    public:
        MemBlock();
        ~MemBlock();

        int Assign(char *pBuf, uint32_t uSize);
        char At(uint32_t uIndex);
        bool GetBit(uint32_t uPos);
        uint32_t GetBitsetCount();
        uint32_t GetBitsetCount(uint32_t uSize);

    public:
        uint32_t    m_uSize;
        char        *m_pBlock;
};

#endif
