#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>
#include <fstream>
#include <map>
#include <string.h>
#include <stdlib.h>
#include "schema.h"

using std::string;
using std::ifstream;
using std::map;
using std::vector;

typedef map<string, string> map_kv_t;
typedef vector< map_kv_t >  vmap_kv_t;
typedef map<string, vmap_kv_t >   map_sec_kv_t;


class Config
{
    public:
        Config(char *szConfigFile);
        ~Config();

        int Load();
        int LoadCommConf();
        int LoadSchemaConf();

        int GetConfigInt(string sSection, string sKey, int nDefault);
        string GetConfigStr(string sSection, string sKey, string sDefault);

        string Trim(string str);

        int PrintCommConf();

        Schema* GetMatchSchema(string sDatabaseName, string sTableName);

    private:
        char                m_szConfigFile[128];
        map_sec_kv_t        m_mapCommConf;
        vector<Schema *>    m_vSchemaConf;

};

extern Config* g_pConfig;
#endif
