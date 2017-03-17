#ifndef SCHEMA_H
#define SCHEMA_H

#include <vector>
#include "myregex.h"

using std::vector;

struct Column
{
    char szColumnName[64];
    int nColumnSeq;
};

class Schema
{
    public:
        Schema(const char *szDatabase, const char *szTable);
        ~Schema();
        int Init();
        int AddColumn(Column *pColumn);
        bool CheckSchema(const char *szDatabase, const char *szTable);
        vector<Column *>& GetColumn();

    private:
        char                m_szDatabase[64];
        char                m_szTable[64];
        vector<Column *>    m_vColumn;
        Regex               *m_pDatabaseRegex;
        Regex               *m_pTableRegex;
};

#endif
