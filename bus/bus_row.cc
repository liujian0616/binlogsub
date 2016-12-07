/***************************************************************************
 * 
 * Copyright (c) 2014 Sina.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
 
 
/**
 * @file bus_row.cc
 * @author liudong2(liudong2@staff.sina.com.cn)
 * @date 2014/05/08 17:50:25
 * @version $Revision$
 * @brief 
 *  
 **/
#include <sys/time.h>
#include <sys/types.h>
#include "bus_row.h"
namespace bus {

void row_t::push_back(const char *p, bool is_old)
{
    if (p == NULL)
    {
        if (is_old)
            _oldcols.push_back(NULL);
        else
            _cols.push_back(NULL);
        return;
    }
    uint32_t sz = strlen(p);
    char *str = new (std::nothrow)char[sz + 1];
    if (str == NULL) oom_error();
    
    if (sz != 0) memcpy(str, p, sz);
    str[sz] = '\0';
    if (is_old) {
        _oldcols.push_back(str);
    } else {
        _cols.push_back(str);
    }
}
void row_t::print(std::string &info)
{
    info.reserve(10240);
    info.append("cols=");
    std::size_t sz = _cols.size();
    for (std::size_t i = 0; i < sz; ++i)
    {
        if (_cols[i] != NULL) info.append(_cols[i]);
        info.append(" ");
    }
    info.append("oldcols=");

    sz = _oldcols.size();
    for(std::size_t i = 0; i < sz; ++i)
    {
        if (_oldcols[i] != NULL) info.append(_oldcols[i]);
        info.append(" ");
    }

}

void row_t::push_back(const char *p, int sz, bool is_old)
{
    char *str = new (std::nothrow)char[sz + 1];
    if (str == NULL) oom_error();
    
    if (sz != 0) memcpy(str, p, sz);
    str[sz] = '\0';
    if (is_old) {
        _oldcols.push_back(str);
    } else {
        _cols.push_back(str);
    }
}

row_t::~row_t()
{
    for(std::vector<char*>::iterator it = _cols.begin();
            it != _cols.end();
            ++it)
    {
        if (*it != NULL)
        {
            delete [] *it;
            *it = NULL;
        }
    }

    for(std::vector<char*>::iterator it = _oldcols.begin();
            it != _oldcols.end();
            ++it)
    {
        if (*it != NULL)
        {
            delete [] *it;
            *it = NULL;
        }
    }
}
}//namespace bus

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
