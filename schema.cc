#include "schema.h"


Schema::Schema(const char *szDatabase, const char *szTable)
{
    strncpy(m_szDatabase, szDatabase, sizeof(m_szDatabase));
    strncpy(m_szTable, szTable, sizeof(m_szTable));
}

Schema::~Schema()
{
    vector<Column *>::iterator it = m_vColumn.begin();
    for (; it != m_vColumn.end(); it++)
    {
        if (*it != NULL)
        {
            delete *it;
            *it = NULL;
        }
    }
}

int Schema::Init()
{
    m_pDatabaseRegex = new Regex();
    if (m_pDatabaseRegex->Init(m_szDatabase) != 0)
        return -1;

    m_pTableRegex = new Regex();
    if (m_pTableRegex->Init(m_szTable) != 0)
        return -1;

    return 0;
}

bool Schema::CheckSchema(const char *szDatabase, const char *szTable)
{
    if (m_pDatabaseRegex->Check(szDatabase) == false
            || m_pTableRegex->Check(szTable) == false)
        return false;

    return true;
}

int Schema::AddColumn(Column *pColumn)
{
    if (pColumn == NULL)
        return -1;

    m_vColumn.push_back(pColumn);
    return 0;
}

vector<Column *>& Schema::GetColumn()
{
    return m_vColumn;
}
