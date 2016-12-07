/***************************************************************************
 * 
 * Copyright (c) 2014 Sina.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
 
 
/**
 * @file bus_row.h
 * @author liudong2(liudong2@staff.sina.com.cn)
 * @date 2014/05/08 16:44:35
 * @version $Revision$
 * @brief 
 *  
 **/

#ifndef  __BUS_ROW_H_
#define  __BUS_ROW_H_
#include <assert.h>
#include <vector>
#include <map>
#include <string>
#include <stdexcept>
#include <exception>
#include <exception>
#include "bus_util.h"
#include "bus_position.h"

namespace bus {
    

enum action_t {INSERT, UPDATE, DEL};

class row_t
{
    public:
        row_t(){};
        explicit row_t(uint32_t sz)
        {
            _cols.reserve(sz);
            _oldcols.reserve(sz);
            //_cmd = NULL;
        }
        row_t(const row_t& other);
        void push_back(const char *p, bool is_old);
        void push_back(const char *p, int sz, bool is_old);

        bool get_value(uint32_t index, char **val_ptr)
        {
            uint32_t sz = _cols.size();
            if (index >= sz) {
                std::string info;
                this->print(info);
                g_logger.error("get value fail, index=%u,size=%u, data=%s",
                        index, sz, info.c_str());
                return false;
            }
            *val_ptr = _cols[index];
            return true;
        }

        bool get_value_byindex(uint32_t index, char **val_ptr)
        {
            uint32_t sz = _cols.size();
            if (index >= sz) {
                return false;
            }
            *val_ptr = _cols[index];
            return true;
        }

        bool get_value_byindex(uint32_t index, char **val_ptr, bus_log_t *solog)
        {
            uint32_t sz = _cols.size();
            if (index >= sz) {
                std::string info;
                this->print(info);
                solog->error("get value fail, index=%u,size=%u, data=%s",
                        index, sz, info.c_str());
                return false;
            }
            *val_ptr = _cols[index];
            return true;
        }

        bool get_value_byname(char* name, char **val_ptr)
        {
            assert(NULL != _pnamemap);
            std::map<std::string, int>::iterator it = (*_pnamemap).find(name);
            if (it == (*_pnamemap).end()) {
                return false;
            }

            uint32_t index = it->second;
            uint32_t sz = _cols.size();
            if (index >= sz) {
                return false;
            }
            *val_ptr = _cols[index];
            return true;
        }
        
        bool get_value_byname(char* name, char **val_ptr, bus_log_t *solog)
        {
            //*solog for _src_schema->get_column_index() need #include"bus_config.h"
            //then bad to bus_process.cc
            assert(NULL != _pnamemap);
            std::map<std::string, int>::iterator it = (*_pnamemap).find(name);
            if (it == (*_pnamemap).end()) {
                solog->error("namemap can not find %s", name);
                return false;
            }

            uint32_t index = it->second;
            uint32_t sz = _cols.size();
            if (index >= sz) {
                std::string info;
                this->print(info);
                solog->error("get value fail, index=%u,size=%u, data=%s",
                        index, sz, info.c_str());
                return false;
            }
            *val_ptr = _cols[index];
            return true;
        }
        
        bool get_old_value(uint32_t index, char **val_ptr)
        {
            uint32_t sz = _oldcols.size();
            if (index >= sz) {
                std::string info;
                this->print(info);
                g_logger.error("get old value fail, index=%u,size=%u, data=%s", index, sz, info.c_str());
                return false;
            }
            *val_ptr = _oldcols[index];
            return true;
        }

        bool get_old_value_byindex(uint32_t index, char **val_ptr)
        {
            uint32_t sz = _oldcols.size();
            if (index >= sz) {
                return false;
            }
            *val_ptr = _oldcols[index];
            return true;
        }

        bool get_old_value_byindex(uint32_t index, char **val_ptr, bus_log_t *solog)
        {
            uint32_t sz = _oldcols.size();
            if (index >= sz) {
                std::string info;
                this->print(info);
                solog->error("get old value fail, index=%u,size=%u, data=%s",
                        index, sz, info.c_str());
                return false;
            }
            *val_ptr = _oldcols[index];
            return true;
        }

        bool get_old_value_byname(char* name, char **val_ptr)
        {
            assert(NULL != _pnamemap);
            std::map<std::string, int>::iterator it = (*_pnamemap).find(name);
            if (it == (*_pnamemap).end()) {
                return false;
            }

            uint32_t index = it->second;
            uint32_t sz = _oldcols.size();
            if (index >= sz) {
                return false;
            }
            *val_ptr = _oldcols[index];
            return true;
        }

        bool get_old_value_byname(char* name, char **val_ptr, bus_log_t *solog)
        {
            assert(NULL != _pnamemap);
            std::map<std::string, int>::iterator it = (*_pnamemap).find(name);
            if (it == (*_pnamemap).end()) {
                solog->error("namemap can not find %s", name);
                return false;
            }

            uint32_t index = it->second;
            uint32_t sz = _oldcols.size();
            if (index >= sz) {
                std::string info;
                this->print(info);
                solog->error("get old value fail, index=%u,size=%u, data=%s",
                        index, sz, info.c_str());
                return false;
            }
            *val_ptr = _oldcols[index];
            return true;
        }

        void set_db(const char *dbname)
        {
            snprintf(_dbname, sizeof(_dbname), "%s", dbname);
        }

        const char *get_db_name()
        {
            return _dbname;
        }

        void set_table(const char *tablename)
        {
            snprintf(_tablename, sizeof(_tablename), "%s", tablename);
        }

        const char* get_table() const
        {
            return _tablename;
        }

        void set_action(action_t action)
        {
            _action = action;
        }

        action_t get_action() const
        {
            return _action;
        }

        uint32_t size() const
        {
            return _cols.size();
        }
        void print(std::string &info);
        ~row_t();
    private:
        /* 行数据，每一个字符串的代表行中的一列 */
        std::vector<char*> _cols;
        std::vector<char*> _oldcols;
        std::map<std::string, int> *_pnamemap;
        /*  表名 */
        char _dbname[64];
        char _tablename[64];
        /* 行的动作 */
        action_t _action;
};
}//namespace bus
#endif  //__BUS_ROW_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
