#include "row.h"

int Row::Init(EnumAction eAction, string sDatabaseName, string sTableName)
{
    m_eAction = eAction;
    m_sDatabaseName = sDatabaseName;
    m_sTableName = sTableName;
    return 0;
}

int Row::PushBack(char *pBuf, bool bOld)
{
    if (bOld)
    {
        m_vOldColumn.push_back(pBuf);
    }
    else
    {
        m_vColumn.push_back(pBuf);
    }
    return 0;
}

int Row::PushBack(string sBuf, bool bOld)
{
    if (bOld)
    {
        m_vOldColumn.push_back(sBuf);
    }
    else
    {
        m_vColumn.push_back(sBuf);
    }
    return 0;
}

int Row::PushBack(char *pBuf, uint32_t uLen, bool bOld)
{
    m_sTemp.assign(pBuf, uLen);
    if (bOld)
    {
        m_vOldColumn.push_back(m_sTemp);
    }
    else
    {
        m_vColumn.push_back(m_sTemp);
    }

    return 0;
}

string& Row::GetColumnByIndex(uint32_t uIndex, bool bOld)
{
    assert(uIndex < m_vColumn.size());
    if (bOld)
        return m_vOldColumn[uIndex];
    else
        return m_vColumn[uIndex];
}

int Row::FilterCopy(Row *pRow)
{
    m_eAction = pRow->m_eAction;
    m_sDatabaseName = pRow->m_sDatabaseName;
    m_sTableName = pRow->m_sTableName;

    Schema *pSchema = g_pConfig->GetMatchSchema(m_sDatabaseName, m_sTableName);
    if (pSchema == NULL)
        return -1;

    vector<Column *> vColumn = pSchema->GetColumn();

    for (uint32_t i = 0; i < vColumn.size(); i++)
    {
        string& sColumnValue = pRow->GetColumnByIndex(vColumn[i]->nColumnSeq, false);
        PushBack(sColumnValue, false);
    }

    if (m_eAction == UPDATE)
    {
        for (uint32_t i = 0; i < vColumn.size(); i++)
        {
            string& sOldColumnValue = pRow->GetColumnByIndex(vColumn[i]->nColumnSeq, true);
            PushBack(sOldColumnValue, true);
        }
    }

    return 0;
}

int Row::Print()
{
    printf("action:%d, oldColumnSize:%d, newColumnSize:%d\n", m_eAction, (int)m_vOldColumn.size(), (int)m_vColumn.size());

    printf("old column start----------\n");
    for (size_t i = 0; i < m_vOldColumn.size(); i++)
    {
        printf("%s\t", m_vOldColumn[i].c_str());
    }
    printf("\n");
    printf("old column end------------\n");

    printf("new column start----------\n");
    for (size_t i = 0; i < m_vColumn.size(); i++)
    {
        printf("%s\t", m_vColumn[i].c_str());
    }
    printf("\n");
    printf("new column end------------\n");
    return 0;
}
