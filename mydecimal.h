
#ifndef MY_DECIMAL_H
#define MY_DECIMAL_H

#include <string.h>
#include "myconvert.h"

#define DIG_PER_DEC1 9

#define E_DEC_TRUNCATED         1
#define E_DEC_OVERFLOW          2

#define E_DEC_FATAL_ERROR      30

#define DECIMAL_BUFF_LENGTH 9


#define swap_variables(t, a, b) { t dummy; dummy= a; a= b; b= dummy; }

#define E_DEC_OK                0

#define UNINIT_VAR(x) x= 0

#define FIX_INTG_FRAC_ERROR(len, intg1, frac1, error)                   \
    do                                                              \
    {                                                               \
    if (unlikely(intg1+frac1 > (len)))                            \
    {                                                             \
        if (unlikely(intg1 > (len)))                                \
        {                                                           \
            intg1=(len);                                              \
            frac1=0;                                                  \
            error=E_DEC_OVERFLOW;                                     \
        }                                                           \
        else                                                        \
        {                                                           \
            frac1=(len)-intg1;                                        \
            error=E_DEC_TRUNCATED;                                    \
        }                                                           \
    }                                                             \
    else                                                          \
    error=E_DEC_OK;                                             \
} while(0)


#define __builtin_expect(x, expected_value) (x)

#define unlikely(x) __builtin_expect((x),0)

#define DIG_BASE     1000000000
#define DIG_MAX      (DIG_BASE-1)

#define decimal_make_zero(dec)        do {                \
    (dec)->buf[0]=0;    \
    (dec)->intg=1;      \
    (dec)->frac=0;      \
    (dec)->sign=0;      \
} while(0)
#define E_DEC_BAD_NUM           8


#define ROUND_UP(X)  (((X)+DIG_PER_DEC1-1)/DIG_PER_DEC1)


typedef int32 decimal_digit_t;

typedef decimal_digit_t dec1;

static const int dig2bytes[DIG_PER_DEC1+1]={0, 1, 1, 2, 2, 3, 3, 4, 4, 4};

static const dec1 powers10[DIG_PER_DEC1+1]={
    1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000};

int decimal_operation_results(int result);

int decimal_bin_size(int precision, int scale);

int my_decimal_get_binary_size(uint precision, uint scale);

int check_result(uint mask, int result);

typedef struct st_decimal_t {
    int    intg, frac, len;
    bool sign;
    decimal_digit_t *buf;
} decimal_t;

class my_decimal :public decimal_t
{
    decimal_digit_t buffer[DECIMAL_BUFF_LENGTH];

    public:

    void init()
    {
        len= DECIMAL_BUFF_LENGTH;
        buf= buffer;
#if !defined (HAVE_purify) && !defined(DBUG_OFF)
        /* Set buffer to 'random' value to find wrong buffer usage */
        for (uint i= 0; i < DECIMAL_BUFF_LENGTH; i++)
            buffer[i]= i;
#endif
    }
    my_decimal()
    {
        init();
    }
    void fix_buffer_pointer() { buf= buffer; }

    bool sign() const { return decimal_t::sign; }
    void sign(bool s) { decimal_t::sign= s; }
    uint precision() const { return intg + frac; }

    /** Swap two my_decimal values */
    void swap(my_decimal &rhs)
    {
        swap_variables(my_decimal, *this, rhs);
        /* Swap the buffer pointers back */
        swap_variables(decimal_digit_t *, buf, rhs.buf);
    }
};


#define my_alloca(A) my_malloc((A),MYF(0))
#define my_afree(A) my_free((A))

int bin2decimal(const unsigned char *from, decimal_t *to, int precision, int scale);

int binary2my_decimal(uint8 mask, const unsigned char *bin, my_decimal *d, int prec, int scale);


#endif
