#include "my_time.h"
#include "myconvert.h"

/*
   On disk we convert from signed representation to unsigned
   representation using TIMEF_OFS, so all values become binary comparable.
 */
#define TIMEF_OFS 0x800000000000LL
#define TIMEF_INT_OFS 0x800000LL


int64_t my_time_packed_from_binary(const char *ptr, uint32_t &pos,  uint32_t dec)
{
    assert(dec <= DATETIME_MAX_DECIMALS);

    switch (dec)
    {
        case 0:
        default:
            {
                int64_t intpart= mi_uint3korr(ptr) - TIMEF_INT_OFS;
                pos += 3;
                return MY_PACKED_TIME_MAKE_INT(intpart);
            }
        case 1:
        case 2:
            {
                int64_t intpart= mi_uint3korr(ptr) - TIMEF_INT_OFS;
                int frac= (uint) ptr[3];
                pos += 3;
                if (intpart < 0 && frac)
                {
                    /*
                       Negative values are stored with reverse fractional part order,
                       for binary sort compatibility.

                       Disk value  intpart frac   Time value   Memory value
                       800000.00    0      0      00:00:00.00  0000000000.000000
                       7FFFFF.FF   -1      255   -00:00:00.01  FFFFFFFFFF.FFD8F0
                       7FFFFF.9D   -1      99    -00:00:00.99  FFFFFFFFFF.F0E4D0
                       7FFFFF.00   -1      0     -00:00:01.00  FFFFFFFFFF.000000
                       7FFFFE.FF   -1      255   -00:00:01.01  FFFFFFFFFE.FFD8F0
                       7FFFFE.F6   -2      246   -00:00:01.10  FFFFFFFFFE.FE7960

                       Formula to convert fractional part from disk format
                       (now stored in "frac" variable) to absolute value: "0x100 - frac".
                       To reconstruct in-memory value, we shift
                       to the next integer value and then substruct fractional part.
                     */
                    intpart++;    /* Shift to the next integer value */
                    frac-= 0x100; /* -(0x100 - frac) */
                }
                return MY_PACKED_TIME_MAKE(intpart, frac * 10000);
            }

        case 3:
        case 4:
            {
                int64_t intpart= mi_uint3korr(ptr) - TIMEF_INT_OFS;
                int frac= mi_uint2korr(ptr + 3);
                pos += 5;
                if (intpart < 0 && frac)
                {
                    /*
                       Fix reverse fractional part order: "0x10000 - frac".
                       See comments for FSP=1 and FSP=2 above.
                     */
                    intpart++;      /* Shift to the next integer value */
                    frac-= 0x10000; /* -(0x10000-frac) */
                }
                return MY_PACKED_TIME_MAKE(intpart, frac * 100);
            }

        case 5:
        case 6:
            pos += 6;
            return ((int64_t) mi_uint6korr(ptr)) - TIMEF_OFS;
    }
}



#define DATETIMEF_INT_OFS 0x8000000000LL

/**
  Convert on-disk datetime representation
  to in-memory packed numeric representation.

  @param ptr   The pointer to read value at.
  @param dec   Precision.
  @return      In-memory packed numeric datetime representation.
 */
int64_t my_datetime_packed_from_binary(const char *ptr, uint32_t &pos, uint32_t dec)
{
    int64_t intpart= mi_uint5korr(ptr) - DATETIMEF_INT_OFS;
    pos += 5;
    int frac;
    assert(dec <= DATETIME_MAX_DECIMALS);
    switch (dec)
    { case 0:
        default:
            return MY_PACKED_TIME_MAKE_INT(intpart);
        case 1:
        case 2:
            frac= ((int) (signed char) ptr[5]) * 10000;
            pos += 1;
            break;
        case 3:
        case 4:
            frac= mi_sint2korr(ptr + 5) * 100;
            pos += 2;
            break;
        case 5:
        case 6:
            frac= mi_sint3korr(ptr + 5);
            pos += 3;
            break;
    }
    return MY_PACKED_TIME_MAKE(intpart, frac);
}

/**   
  Convert binary timestamp representation to in-memory representation.

  @param  OUT tm  The variable to convert to.
  @param      ptr The pointer to read the value from.
  @param      dec Precision.
 */    
void my_timestamp_from_binary(struct timeval *tm, const char *ptr, uint32_t &pos, uint32_t dec)
{     
    assert(dec <= DATETIME_MAX_DECIMALS);
    tm->tv_sec= mi_uint4korr(ptr);
    pos += 4;
    switch (dec)
    {   
        case 0:
        default:
            tm->tv_usec= 0;
            break;
        case 1:
        case 2:
            tm->tv_usec= ((int) ptr[4]) * 10000;
            pos += 4;
            break; 
        case 3:
        case 4:       
            tm->tv_usec= mi_sint2korr(ptr + 4) * 100; 
            pos += 2;
            break;      
        case 5:       
        case 6:       
            tm->tv_usec= mi_sint3korr(ptr + 4);
            pos += 3;
    }
}

