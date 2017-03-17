#ifndef REGEX_H
#define REGEX_H
#include <regex.h>
#include <string.h>

class Regex
{
    public:
        Regex();
        ~Regex();

        int Init(const char *szPattern);
        bool Check(const char *szText);

    private:
        regex_t m_reg;
        char    m_szPattern[64];
};


#endif
