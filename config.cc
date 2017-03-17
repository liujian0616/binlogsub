#include "config.h"


Config::Config(char *szConfigFile)
{
    strncpy(m_szConfigFile, szConfigFile, sizeof(m_szConfigFile));
}

Config::~Config()
{

}

string Config::Trim(string str)
{
    size_t pos = 0;
    pos = str.find(" ", pos);
    while (pos !=  string::npos)
    {
        str.replace(pos, 1, "");
        pos = str.find(" ", pos);
    }

    return str;
}

int Config::LoadCommConf()
{
    ifstream fin(m_szConfigFile);
    string strLine;
    string sCurSection;
    string sKey;
    string sValue;
    vmap_kv_t  vmap_kv;

    while (getline(fin, strLine))
    {
        if (strLine.empty() || strLine[0] == '\n' || strLine[0] == '\r' || strLine[0] == '#')
            continue;

        strLine = Trim(strLine);

        if (strLine[0] == '[' && strLine[ strLine.size() - 1 ] == ']')
        {
            if (sCurSection.empty() == false )
            {
                if (sCurSection.find("schema") == string::npos)
                {
                    /*
                     *@todo 发现新的section，先将上一个section下的 kv列 保存好
                     */
                    m_mapCommConf.insert(make_pair(sCurSection, vmap_kv));
                    vmap_kv.clear();
                }
            }

            sCurSection = strLine.substr(1, strLine.size() - 2);
            continue;
        }

        size_t pos = strLine.find("=");
        if (pos != string::npos)
        {
            sKey = strLine.substr(0, pos);
            sValue = strLine.substr(pos + 1, strLine.size() - pos - 1);

            map_kv_t map_kv;
            map_kv.insert(make_pair(sKey, sValue));
            vmap_kv.push_back(map_kv);
        }

    }

    return 0;
}

int Config::LoadSchemaConf()
{
     ifstream fin(m_szConfigFile);
    string strLine;
    string sCurSection;
    string sKey;
    string sValue;
    string sColumnName;
    string sColumnIndex;
    string sDatabase;
    string sTable;
    Schema *pSchema = NULL;
    Column *pColumn = NULL;

    sCurSection.clear();

    while (getline(fin, strLine))
    {
        if (strLine.empty() || strLine[0] == '\n' || strLine[0] == '\r' || strLine[0] == '#')
            continue;

        strLine = Trim(strLine);
        if (strLine[0] == '[' && strLine[ strLine.size() - 1 ] == ']')
        {
            if (strLine.find("schema") != string::npos)
                sCurSection = strLine.substr(1, strLine.size() - 2);

            continue;

        }

        if (sCurSection.empty())
            continue;

        size_t pos = strLine.find("=");
        if (pos != string::npos)
        {
            sKey = strLine.substr(0, pos);
            sValue = strLine.substr(pos + 1, strLine.size() - pos - 1);

            if (sCurSection.compare("schema") == 0)
            {
                if (sKey.compare("database") == 0)
                    sDatabase = sValue;

                if (sKey.compare("table") == 0)
                    sTable = sValue;

                if (sDatabase.empty() == false && sTable.empty() == false)
                {
                    pSchema = new Schema(sDatabase.c_str(), sTable.c_str());
                    pSchema->Init();

                    m_vSchemaConf.push_back(pSchema);
                    printf("new schema: %s, %s, %s\n", sCurSection.c_str(), sDatabase.c_str(), sTable.c_str());

                    sDatabase.clear();
                    sTable.clear();
                }
            }

            if (sCurSection.compare(0, 14, "schema_column_") == 0)
            {
                if (sKey.compare("column_name") == 0)
                    sColumnName = sValue;
                if (sKey.compare("column_index") == 0)
                    sColumnIndex = sValue;
                if (sColumnName.empty() == false && sColumnIndex.empty() == false)
                {
                    pColumn = new Column;
                    strncpy(pColumn->szColumnName, sColumnName.c_str(), sizeof(pColumn->szColumnName));
                    pColumn->nColumnSeq = atoi(sColumnIndex.c_str());
                    pSchema->AddColumn(pColumn);
                    printf("add column: %d, %s\n", pColumn->nColumnSeq, pColumn->szColumnName);

                    sColumnName.clear();
                    sColumnIndex.clear();
                }
            }
        }

    }

    return 0;
}

int Config::Load()
{
    LoadCommConf();
    LoadSchemaConf();

    return 0;
}

int Config::PrintCommConf()
{
    map_sec_kv_t::iterator it1 = m_mapCommConf.begin();
    for (; it1 != m_mapCommConf.end(); it1++)
    {
        printf("%s\n", it1->first.c_str());

        vmap_kv_t::iterator it2 = it1->second.begin();
        for (; it2 != it1->second.end(); it2++)
        {
            map_kv_t::iterator it3 = (*it2).begin();;
            printf("    %s = %s\n", it3->first.c_str(), it3->second.c_str());
        }
    }

    return 0;
}

int Config::GetConfigInt(string sSection, string sKey, int nDefault)
{
    map_sec_kv_t::iterator it1 = m_mapCommConf.begin();
    for (; it1 != m_mapCommConf.end(); it1++)
    {
        if (it1->first.compare(sSection) != 0)
            continue;

        vmap_kv_t::iterator it2 = it1->second.begin();
        for (; it2 != it1->second.end(); it2++)
        {
            map_kv_t::iterator it3 = (*it2).begin();;
            if (it3->first.compare(sKey) != 0)
                continue;

            return atoi(it3->second.c_str());
        }
    }

    return nDefault;
}

string Config::GetConfigStr(string sSection, string sKey, string sDefault)
{
    map_sec_kv_t::iterator it1 = m_mapCommConf.begin();
    for (; it1 != m_mapCommConf.end(); it1++)
    {
        if (it1->first.compare(sSection) != 0)
            continue;

        vmap_kv_t::iterator it2 = it1->second.begin();
        for (; it2 != it1->second.end(); it2++)
        {
            map_kv_t::iterator it3 = (*it2).begin();;
            if (it3->first.compare(sKey) != 0)
                continue;

            return it3->second;
        }
    }

    return sDefault;
}


Schema* Config::GetMatchSchema(string sDatabaseName, string sTableName)
{
    for (size_t i = 0; i < m_vSchemaConf.size(); i++)
    {
        if (m_vSchemaConf[i]->CheckSchema(sDatabaseName.c_str(), sTableName.c_str()) == true)
            return m_vSchemaConf[i];
    }
    return NULL;
}
