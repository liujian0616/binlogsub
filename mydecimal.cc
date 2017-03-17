
#include "mydecimal.h"


int decimal_operation_results(int result)
{
    return result;
}

int decimal_bin_size(int precision, int scale)
{
    int intg=precision-scale,
        intg0=intg/DIG_PER_DEC1, frac0=scale/DIG_PER_DEC1,
        intg0x=intg-intg0*DIG_PER_DEC1, frac0x=scale-frac0*DIG_PER_DEC1;

    return intg0*sizeof(dec1)+dig2bytes[intg0x]+
        frac0*sizeof(dec1)+dig2bytes[frac0x];
}

int my_decimal_get_binary_size(uint precision, uint scale)
{
    return decimal_bin_size((int)precision, (int)scale);
}

int check_result(uint mask, int result)
{
    if (result & mask)
        decimal_operation_results(result);
    return result;
}


int bin2decimal(const unsigned char *from, decimal_t *to, int precision, int scale)
{
    int error=E_DEC_OK, intg=precision-scale,
        intg0=intg/DIG_PER_DEC1, frac0=scale/DIG_PER_DEC1,
        intg0x=intg-intg0*DIG_PER_DEC1, frac0x=scale-frac0*DIG_PER_DEC1,
        intg1=intg0+(intg0x>0), frac1=frac0+(frac0x>0);
    dec1 *buf=to->buf, mask=(*from & 0x80) ? 0 : -1;
    const unsigned char *stop;
    unsigned char *d_copy;
    int bin_size= decimal_bin_size(precision, scale);

    //  sanity(to);
    d_copy= (unsigned char*) malloc(bin_size);
    memset(d_copy,0,bin_size);
    memcpy(d_copy, from, bin_size);
    d_copy[0]^= 0x80;
    from= d_copy;

    FIX_INTG_FRAC_ERROR(to->len, intg1, frac1, error);
    if (unlikely(error))
    {
        if (intg1 < intg0+(intg0x>0))
        {
            from+=dig2bytes[intg0x]+sizeof(dec1)*(intg0-intg1);
            frac0=frac0x=intg0x=0;
            intg0=intg1;
        }
        else
        {
            frac0x=0;
            frac0=frac1;
        }
    }

    to->sign=(mask != 0);
    to->intg=intg0*DIG_PER_DEC1+intg0x;
    to->frac=frac0*DIG_PER_DEC1+frac0x;

    if (intg0x)
    {
        int i=dig2bytes[intg0x];
        dec1 UNINIT_VAR(x);
        switch (i)
        {
            case 1: x=mi_sint1korr(from); break;
            case 2: x=mi_sint2korr(from); break;
            case 3: x=mi_sint3korr(from); break;
            case 4: x=mi_sint4korr(from); break;
        }
        from+=i;
        *buf=x ^ mask;
        if (((uint64)*buf) >= (uint64) powers10[intg0x+1])
            goto err;
        if (buf > to->buf || *buf != 0)
            buf++;
        else
            to->intg-=intg0x;
    }
    for (stop=from+intg0*sizeof(dec1); from < stop; from+=sizeof(dec1))
    {
        *buf=mi_sint4korr(from) ^ mask;
        if (((uint32)*buf) > DIG_MAX)
            goto err;
        if (buf > to->buf || *buf != 0)
            buf++;
        else
            to->intg-=DIG_PER_DEC1;
    }
    for (stop=from+frac0*sizeof(dec1); from < stop; from+=sizeof(dec1))
    {
        *buf=mi_sint4korr(from) ^ mask;
        if (((uint32)*buf) > DIG_MAX)
            goto err;
        buf++;
    }
    if (frac0x)
    {
        int i=dig2bytes[frac0x];
        dec1 UNINIT_VAR(x);
        switch (i)
        {
            case 1: x=mi_sint1korr(from); break;
            case 2: x=mi_sint2korr(from); break;
            case 3: x=mi_sint3korr(from); break;
            case 4: x=mi_sint4korr(from); break;
        }
        *buf=(x ^ mask) * powers10[DIG_PER_DEC1 - frac0x];
        if (((uint32)*buf) > DIG_MAX)
            goto err;
        buf++;
    }
    free(d_copy);
    return error;

err:
    free(d_copy);
    decimal_make_zero(((decimal_t*) to));
    return(E_DEC_BAD_NUM);
}


int binary2my_decimal(uint8 mask, const unsigned char *bin, my_decimal *d, int prec, int scale)
{
    return check_result(mask, bin2decimal(bin, (decimal_t*) d, prec, scale));
}

