/***************************************************************************
 * 
 * Copyright (c) 2014 Sina.com, Inc. All Rights Reserved
 * 
 **************************************************************************/


/**
 * @file bus_resource.h
 * @author liudong2(liudong2@staff.sina.com.cn)
 * @date 2014/04/14 20:57:36
 * @version $Revision$
 * @brief 主要数据结构定义
 * 
 **/

#ifndef  __BUS_RESOURCE_H_
#define  __BUS_RESOURCE_H_
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <vector>
#include <map>
#include <string>
#include "bus_log.h"
#include "bus_util.h"
#include "tinyxml/tinyxml2.h"
#include "bus_position.h"
#include </usr/include/regex.h>


namespace bus {
#define BUS_SUCC 0
#define BUS_FAIL -1
    class column_t
    {
        public:
            column_t(const char *column_name):_type(-1),_seq(-1)
        {
            snprintf(_column_name, sizeof(_column_name), "%s", column_name);
        }

            column_t(const char *column_name, int32_t seq, 
                    int32_t type):_type(type), _seq(seq) 
        {
            snprintf(_column_name, sizeof(_column_name), "%s", column_name);
        }

            const char* get_column_name() const
            {
                return _column_name;
            }

            uint32_t get_column_type() const
            {
                return _type;
            }

            void set_column_type(uint32_t type)
            {
                _type = type;
            }

            uint32_t get_column_seq() const
            {
                return _seq;
            }

            void set_column_seq(uint32_t seq)
            {
                _seq = seq;
            }

        private:
            char _column_name[64];
            uint32_t _type;
            // 此列在行中的位置 
            uint32_t _seq;
    };

    class bus_regex_t
    {
        public:
            explicit bus_regex_t()
            {
                _ok = false;
                _match_flag = REG_EXTENDED;
            }

            bool init(const char *pattern)
            {
                if (pattern == NULL) {
                    g_logger.error("pattern is NULL");
                    return false;
                }
                snprintf(_pattern, sizeof(_pattern), "%s", pattern);
                char ebuffer[128];
                int ret = regcomp(&_reg, _pattern, _match_flag);
                if (ret != 0)
                {
                    regerror(ret, &_reg, ebuffer, sizeof(ebuffer));
                    g_logger.error("compile regex=%s fail, error:%s", _pattern, ebuffer);
                    return false;
                }
                _ok = true;
                return true;
            }

            bool check(const char *text)
            {
                assert(text != NULL);
                if (!_ok)
                {
                    g_logger.error("regex should be init first");
                    return false;
                }
                size_t nmatch = 1;
                regmatch_t pmatch[1];
                int status = regexec(&_reg, text, nmatch, pmatch, 0);
                if(status == 0)
                {
                    return true;
                }
                return false;
            }

            ~bus_regex_t() 
            {
                if (_ok)
                {
                    regfree(&_reg);
                    _ok = false;
                }
            };

            bool is_ok()
            {
                return _ok;
            }
        private:
            bool _ok;
            regex_t _reg;
            int _match_flag;
            char _pattern[64];
    };

    /**
     * @brief 表元数据数据结构
     */
    class schema_t
    {
        public:
            schema_t(const long id, const char *name, const char *db);
            ~schema_t();

            void add_column(column_t* col);
#if 1
            const char* get_schema_db() const
            {
                return _db;
            }
            const char* get_schema_table() const
            {
                return _table;
            }
#endif
            std::vector<column_t*>& get_columns()
            {   
                return _columns;
            }   

            bool init_regex();

            bool check_schema(const char *dbname, const char *tablename);
            column_t *get_column_byseq(int seq)
            {
                std::size_t sz = _columns.size();
                for (uint32_t i = 0; i < sz; ++i)
                {
                    int col_seq = _columns[i]->get_column_seq();
                    if (seq == col_seq) return _columns[i];
                }
                return NULL;
            }

        private:
            std::vector<column_t*> _columns;

            /* table的名称 */
            char _table[64];
            char _db[64];
            long _schema_id;

            /* 正则表达式对象 */
            bus_regex_t _table_name_regex;
            bus_regex_t _db_name_regex;
    };

    /**
     * @brief 配置信息数据结构
     */
    class bus_config_t
    {
        public:
            bus_config_t();
            ~bus_config_t();

            std::vector<schema_t*>& get_src_schema();
            schema_t* get_match_schema(const char *db_name, const char *table_name);

            const char* get_target_charset() 
            {
                return _target_charset;
            }
            const char* get_mysql_ip()
            {
                return _mysql_ip;
            }
            int get_mysql_port()
            {
                return _mysql_port;
            }
            int get_mysql_serverid()
            {
                return _mysql_serverid;
            }
            const char* get_mysql_username()
            {
                return _mysql_username;
            }
            const char* get_mysql_userpasswd()
            {
                return _mysql_userpasswd;
            }
            const char* get_redis_ip()
            {
                return _redis_ip;
            }
            int get_redis_port()
            {
                return _redis_port;
            }
            
            
            int init_conffile(const char* config_file);
            int init_schema_regex();

            char* trim(char *str);
            int parase_config_file();
        private:
            DISALLOW_COPY_AND_ASSIGN(bus_config_t);

            std::vector<schema_t*>  _source_schema;
        
        public:
            char                    _target_charset[16];
            char                    _logfile[128];
            char                    _config_file[128];
            char                    _binlog_prefix[64];
            char                    _mysql_ip[128];
            int                     _mysql_port;
            int                     _mysql_serverid;
            char                    _mysql_username[128];
            char                    _mysql_userpasswd[128];
            bool                    _password_need_decode;
            char                    _redis_ip[128];
            int                     _redis_port;
            int                     _redis_switch;
            bool                    _redis_need_passwd;
            char                    _redis_passwd[128];
    };


} //namespace bus


#endif  //__BUS_RESOURCE_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
