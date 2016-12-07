/***************************************************************************
 * 
 * Copyright (c) 2014 Sina.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
 
 
/**
 * @file bus_position.h
 * @author liudong2(liudong2@staff.sina.com.cn)
 * @date 2014/06/23 12:34:46
 * @version $Revision$
 * @brief 
 *  
 **/
#ifndef  __BUS_POSITION_H_
#define  __BUS_POSITION_H_
#include<string>
#include "bus_log.h"
namespace bus {

class mysql_position_t
{
    public:
        mysql_position_t()
        {
            binlog_file[0] = '\0';
        }
        mysql_position_t(const mysql_position_t &other)
        {
            if (other.binlog_file[0] != '\0') {
                snprintf(binlog_file, sizeof(binlog_file), "%s", other.binlog_file);
                binlog_offset = other.binlog_offset;
            } else {
                binlog_file[0] = '\0';
            }
        }

        bool empty()
        {
            return binlog_file[0] == '\0' ? true : false;
        }
        
        void reset_position()
        {
            binlog_file[0] = '\0';
        }

        bool set_position(const char *binfile, long offset)
        {
            snprintf(binlog_file, sizeof(binlog_file), binfile);
            binlog_offset = offset;

            std::string filename(binlog_file);
            std::size_t sz = filename.size();
            std::size_t sep_pos = filename.rfind('.');
            if (sep_pos == std::string::npos)
            {
                g_logger.error("[binlog_filename=%s] is invalid", binlog_file);
                return false;
            }

            for (std::size_t index = sep_pos + 1; index < sz; ++index)
            {
                if (filename[index] < '0' || filename[index] > '9')
                {
                    g_logger.error("[binlog_filename=%s] is invalid", binlog_file);
                    return false;
                }
            }
            return true;
        }

        void set_offset(long offset)
        {
            binlog_offset = offset;
        }

        int compare_position(mysql_position_t &other)
        {
            return compare_position(other.binlog_file, other.binlog_offset);
        }
        int compare_position(const char *filename, long offset)
        {
            int ret = strcasecmp(binlog_file, filename);
            if (ret > 0)
            {
                return 1;
            } else if (ret == 0) 
            {
                if (binlog_offset > offset)
                {
                    return 1;
                }             
            }
            return -1;
        }
    public:
        char binlog_file[64];
        long binlog_offset;
};
}//namespace bus

#endif  //__BUS_POSITION_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
