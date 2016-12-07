/***************************************************************************
 * 
 * Copyright (c) 2014 Sina.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
 
/**
 * @file bus_resource.cc
 * @author liudong2(liudong2@staff.sina.com.cn)
 * @date 2014/04/15 15:34:05
 * @version $Revision$
 * @brief 实现bus_resource定义
 *  
 **/
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "bus_config.h"
#include "bus_position.h"

namespace bus{

bus_config_t g_config;


/******************************************************/
schema_t::schema_t(const long id, const char *db, const char *name) : _schema_id(id)
{
    snprintf(_table, sizeof(_table), "%s", name);
    if(db != NULL)
    {
        snprintf(_db, sizeof(_db), "%s", db);
    } else 
    {
        _db[0] = '\0';
    }
}

void schema_t::add_column(column_t* col)
{
    _columns.push_back(col);
}

bool schema_t::init_regex()
{
    /* 编译正则表达式 */
    if (_table[0] == '\0' || !_table_name_regex.init(_table))
    {
        g_logger.error("table name=%s regex init fail", _table);
        return false;
    }

    if (_db[0] == '\0' || !_db_name_regex.init(_db))
    {
        g_logger.error("db name=%s regex init fail", _db);
        return false;
    }
    return true;
}

bool schema_t::check_schema(const char *dbname, const char *tablename)
{
    if (!_table_name_regex.is_ok() || !_db_name_regex.is_ok())
    {
        g_logger.error("[%s.%s] regex is not ok", _db, _table);
        return false;
    }
    if (!_table_name_regex.check(tablename))
    {
        return false;
    }

    if (!_db_name_regex.check(dbname))
    {
        return false;
    }

    return true;
}

schema_t::~schema_t()
{
    //delete other rows
    for (std::vector<column_t*>::iterator it = _columns.begin(); 
            it != _columns.end(); 
            ++it)
    {
        if (*it != NULL)
        {
            delete *it;
            *it = NULL;
        }
    }
}

/******************************************************/
bus_config_t::bus_config_t()
{
    memset(_target_charset, 0, sizeof(_target_charset));
    memset(_logfile, 0, sizeof(_logfile));
    memset(_config_file, 0, sizeof(_config_file));
    memset(_binlog_prefix, 0, sizeof(_binlog_prefix));
    memset(_mysql_ip, 0, sizeof(_mysql_ip));
    _mysql_port = 0;
    _mysql_serverid = 0;
    memset(_mysql_username, 0, sizeof(_mysql_username));
    memset(_mysql_userpasswd, 0, sizeof(_mysql_userpasswd));
    memset(_redis_ip, 0, sizeof(_redis_ip));
    _redis_port = 0;

    strncpy(_target_charset, "utf-8", sizeof(_target_charset));
}

bus_config_t::~bus_config_t()
{

    for (std::vector<schema_t*>::iterator it =  _source_schema.begin(); it != _source_schema.end(); ++it)
    {
        if (*it != NULL) delete *it;
    }

}

int bus_config_t::init_conffile(const char* config_file)
{
    if (config_file[0] == 0)
        return -1;

    snprintf(_config_file, sizeof(_config_file), "%s", config_file);

    return 0;
}

schema_t* bus_config_t::get_match_schema(const char *db_name, const char *table_name)
{
    for (std::vector<schema_t*>::iterator it = _source_schema.begin(); it != _source_schema.end(); ++it)
    {
        schema_t *curschema = *it;
        if (curschema->check_schema(db_name, table_name))
        {
            return curschema;
        }
    }

    return NULL;
}

int bus_config_t::init_schema_regex()
{
    for (std::vector<schema_t*>::iterator it = _source_schema.begin(); it != _source_schema.end(); ++it)
    {
        schema_t *curschema = *it;
        if (!curschema->init_regex())
        {
            g_logger.error("init regex schema=%s, table=%s fail", curschema->get_schema_db(), curschema->get_schema_table());
            return -1;
        }
    }
    return 0;
}


std::vector<schema_t*>& bus_config_t::get_src_schema()
{
    return _source_schema;
}


char * bus_config_t::trim(char *str)
{
    char *pBegin = str;
    int nLen = strlen(str);

    for (int i = nLen -1; i >= 0; i--)
    {
        if (str[i] == ' ' || str[i] == '\t' || str[i] == '\n')
        {
            str[i] = 0;
        }
        else
        {
            break;
        }
    }

    for (int i = 0; i < nLen; i++)
    {
        if (str[i] == ' ' || str[i] == '\t' || str[i] == '\n') 
        {
            pBegin++;
        }
        else
        {
            break;
        }
    }
    return pBegin;
}

int bus_config_t::parase_config_file()
{
    char szLine[1024] = {0};
    char *szConfLine = NULL;

    char szMainItem[100] = {0};
    char szSubItem[100] = {0};
    char szKey[500] = {0};
    char szValue[500] = {0};

    char szDatabase[100] = {0};
    char szTable[100] = {0};
    char szColumnName[100] = {0};
    int nColumnIndex = -1;

    schema_t* pSchema = NULL;
    int nSchemaIndex = 0;

    FILE *fp = NULL;
    if (_config_file[0] != 0)
    {
        fp = fopen(_config_file, "r");
    }
    else
    {
        g_logger.error("please give configFile");
        return -1;
    }

    if (fp == NULL)
    {
        g_logger.error("fopen error");
        return -1;
    }
    while (!feof(fp))
    {
        fgets(szLine, 1024, fp);
        //printf("szLine:  %s\n", szLine);
        szConfLine = trim(szLine);
        //printf("szConfLine:  %s\n", szConfLine);

        if (szConfLine[0] == 0)
            continue;

        int conf_line_len = strlen(szConfLine);
        if (szConfLine[0] == '[' && szConfLine[conf_line_len - 1] == ']')
        {
            if (szConfLine[conf_line_len - 2] < '0' || szConfLine[conf_line_len - 2] > '9') 
            {
                memset(szMainItem, 0, 100);
                memcpy(szMainItem, szConfLine + 1, conf_line_len - 2);
                memset(szSubItem, 0, 100);
                continue;
            }

            memset(szSubItem, 0, 100);
            memcpy(szSubItem, szConfLine + 1, conf_line_len - 2);
            continue;

        }

        /*
         *@todo 除了 main_item 和 sub_item, 剩下的就是 key=value 键值对了
         */
#if 0
        char *result = strtok(szConfLine, "=");
        if (result != NULL)
            strncpy(szKey, trim(result), sizeof(szKey));

        result = strtok(NULL, "=");
        if (result != NULL)
            strncpy(szValue, trim(result), sizeof(szKey));
#endif
        char *p = strstr(szConfLine, "=");
        if (p != NULL)
        {
            *p++ = 0;
            strncpy(szKey, trim(szConfLine), sizeof(szKey));
            strncpy(szValue, trim(p), sizeof(szValue));
        }
        else
        {
            continue;
        }


        if (strcmp(szMainItem, "mysql") == 0)
        {
            g_logger.debug("%s, %s, %s", szMainItem, szKey, szValue);
            if (strcmp(szKey, "mysql_ip") == 0)
            {
                strncpy(_mysql_ip, szValue, sizeof(_mysql_ip));
                g_logger.notice("read mysql_ip: %s", _mysql_ip);
            }

            else if (strcmp(szKey, "mysql_port") == 0)
            {
                _mysql_port = atoi(szValue);
                g_logger.notice("read mysql_port: %d", _mysql_port);
            }

            else if (strcmp(szKey, "mysql_serverid") == 0)
            {
                _mysql_serverid = atoi(szValue);
                g_logger.notice("mysql_serverid: %d", _mysql_serverid);
            }
            else if (strcmp(szKey, "username") == 0)
            {
                strncpy(_mysql_username, szValue, sizeof(_mysql_username));
                g_logger.notice("read mysql_username: %s", _mysql_username);
            }
            else if (strcmp(szKey, "password") == 0)
            {
                strncpy(_mysql_userpasswd, szValue, sizeof(_mysql_userpasswd));
                g_logger.notice("read mysql_userpasswd: %s", _mysql_userpasswd);
            }
            else if (strcmp(szKey, "password_need_decode") == 0)
            {
                _password_need_decode = (atoi(szValue) == 0) ? false : true;
                g_logger.notice("read password_need_decode: %d", _password_need_decode);
            }

        }
        else if(strcmp(szMainItem, "redis") == 0)
        {
            g_logger.debug("%s, %s, %s", szMainItem, szKey, szValue);
            if (strcmp(szKey, "redis_ip") == 0)
            {
                strncpy(_redis_ip, szValue, sizeof(_redis_ip));
                g_logger.notice("read redis_ip: %s", _redis_ip);
            }
            else if (strcmp(szKey, "redis_port") == 0)
            {
                _redis_port = atoi(szValue);
                g_logger.notice("read redis_port: %d", _redis_port);
            }
            else if (strcmp(szKey, "switch") == 0)
            {
                _redis_switch = atoi(szValue);
                g_logger.notice("read redis_switch: %d", _redis_switch);
            }
            else if (strcmp(szKey, "password") == 0)
            {
                if (szValue[0] == 0)
                {
                    _redis_need_passwd = false;
                }
                else
                {
                    _redis_need_passwd = true;
                }

                strncpy(_redis_passwd, szValue, sizeof(_redis_passwd));
                g_logger.notice("read redis_passwd: %s", _redis_passwd);
                g_logger.notice("read redis_need_passwd: %d", _redis_need_passwd);
            }

        }
        else if (strcmp(szMainItem, "bus_schema") == 0)
        {
            if (strncmp(szSubItem, "bus_schema_column_", 18) == 0)
            {
                g_logger.debug("%s, %s, %s, %s", szMainItem, szSubItem, szKey, szValue);
                if (strcmp(szKey, "column_index") == 0)
                    nColumnIndex = atoi(szValue);
                if (strcmp(szKey, "column_name") == 0)
                    strncpy(szColumnName, szValue, sizeof(szColumnName));

                if (nColumnIndex != -1 && szColumnName[0] != 0)
                {
                    column_t* pColumn = new bus::column_t(szColumnName, nColumnIndex, -1);
                    if (pSchema != NULL)
                    {
                        pSchema->add_column(pColumn);
                        g_logger.notice("new column, columnName:%s, columnIndex:%d, pschema:%p", szColumnName, nColumnIndex, pSchema);
                    }

                    nColumnIndex = -1;
                    memset(szColumnName, 0, sizeof(szColumnName)); 
                }

            }
            else
            {
                g_logger.debug("%s, %s, %s", szMainItem, szKey, szValue);
                if (strcmp(szKey, "database") == 0)
                    strncpy(szDatabase, szValue, sizeof(szDatabase));
                if (strcmp(szKey, "table") == 0)
                    strncpy(szTable, szValue, sizeof(szDatabase));

                if (szDatabase[0] != 0 && szTable[0] != 0)
                {
                    pSchema = new schema_t(nSchemaIndex++, szDatabase, szTable);
                    if (pSchema == NULL || pSchema->init_regex() == false)
                        g_logger.error("fail to new schema");
                    
                    _source_schema.push_back(pSchema);
                    g_logger.notice("new schema: database:%s, table:%s, pSchema:%p", szDatabase, szTable, pSchema);
                    memset(szDatabase, 0, sizeof(szDatabase));
                    memset(szTable, 0, sizeof(szTable));
                }
            }
        }

    }
    return 0;
}


}//namespace bus

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
