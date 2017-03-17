#ifndef ROW_H
#define ROW_H

#include <stdint.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <assert.h>
#include "config.h"
using std::vector;
using std::string;

enum EnumAction
{
    INSERT,
    UPDATE,
    DEL
};

/*
 *@todo 存储解析后的行数据
 */
class Row
{
    public:
        Row() {};
        ~Row() {};
        int Init(EnumAction eAction, string sDatabaseName, string sTableName);
        int PushBack(char *pBuf, bool bOld);
        int PushBack(string sBuf, bool bOld);
        int PushBack(char *pBuf, uint32_t uLen, bool bOld);

        string& GetColumnByIndex(uint32_t uIndex, bool bOld);

        int FilterCopy(Row *pRow);
        int Print();

    public:
        EnumAction      m_eAction;
        vector<string>  m_vColumn;
        vector<string>  m_vOldColumn;
        string          m_sDatabaseName;
        string          m_sTableName;
        string          m_sTemp;
};

#endif
