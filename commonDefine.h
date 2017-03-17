#ifndef COMMONDEFINE_H
#define COMMONDEFINE_H

#define PACKET_NORMAL_SIZE 0xffffff

#define MYSQL_TYPE_DECIMAL  0
#define MYSQL_TYPE_TINY  1          
#define MYSQL_TYPE_SHORT  2
#define MYSQL_TYPE_LONG  3
#define MYSQL_TYPE_FLOAT  4
#define MYSQL_TYPE_DOUBLE  5
#define MYSQL_TYPE_NULL  6
#define MYSQL_TYPE_TIMESTAMP 7
#define MYSQL_TYPE_LONGLONG  8
#define MYSQL_TYPE_INT24  9
#define MYSQL_TYPE_DATE  10
#define MYSQL_TYPE_TIME  11
#define MYSQL_TYPE_DATETIME 12
#define MYSQL_TYPE_YEAR 13
#define MYSQL_TYPE_NEWDATE 14
#define MYSQL_TYPE_VARCHAR  15
#define MYSQL_TYPE_BIT  16
#define MYSQL_TYPE_NEWDECIMAL 246
#define MYSQL_TYPE_ENUM 247
#define MYSQL_TYPE_SET 248
#define MYSQL_TYPE_TINY_BLOB 249
#define MYSQL_TYPE_MEDIUM_BLOB 250
#define MYSQL_TYPE_LONG_BLOB 251
#define MYSQL_TYPE_BLOB 252
#define MYSQL_TYPE_VAR_STRING 253
#define MYSQL_TYPE_STRING  254
#define MYSQL_TYPE_GEOMETRY 255

enum enum_binlog_checksum_alg {
    BINLOG_CHECKSUM_ALG_OFF= 0,    // Events are without checksum though its generator  is checksum-capable New Master (NM).
    BINLOG_CHECKSUM_ALG_CRC32= 1,  // CRC32 of zlib algorithm.
    BINLOG_CHECKSUM_ALG_ENUM_END,  // the cut line: valid alg range is [1, 0x7f].
    BINLOG_CHECKSUM_ALG_UNDEF= 255 // special value to tag undetermined yet checksum  or events from checksum-unaware servers
};

#define CHECKSUM_CRC32_SIGNATURE_LEN 4 
/**
  defined statically while there is just one alg implemented
 */
#define BINLOG_CHECKSUM_LEN CHECKSUM_CRC32_SIGNATURE_LEN
#define BINLOG_CHECKSUM_ALG_DESC_LEN 1  /* 1 byte checksum alg descriptor */


#define DATETIME_MAX_DECIMALS 6
#ifndef MYSQL_TYPE_TIMESTAMP2
#define MYSQL_TYPE_TIMESTAMP2 17
#endif

#ifndef MYSQL_TYPE_DATETIME2
#define MYSQL_TYPE_DATETIME2 18
#endif

#ifndef MYSQL_TYPE_TIME2
#define MYSQL_TYPE_TIME2 19
#endif

typedef struct tagContext tagContext;

#endif
