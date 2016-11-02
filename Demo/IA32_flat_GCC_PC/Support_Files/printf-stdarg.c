#include <stdarg.h>
#include <stdint.h>
#include "pc_support.h"

static void my_puts(const char *s)
{
    unsigned int i;
    for (i = 0; s[i] != '\0'; i++) vScreenPutchar(s[i]);
}

static void my_putn(uint32_t n, uint32_t base)
{
    char n2c[] = "0123456789abcdef";
    char s[20], *ps;

    if (n == 0) vScreenPutchar('0');
    else {
        s[19] = '\0';
        ps = s + 18;
        for (; n; n /= base, ps--) *ps = n2c[n % base];
        my_puts(ps + 1);
    }
}

void printf(const char *format, ...)
{
    va_list args;
    unsigned int i;
    int32_t n;

    va_start(args, format);
    for (i = 0; format[i]; i++) {
        if (format[i] == '%') {
            i++;
            if (format[i] == 'd') {
                n = va_arg(args, int32_t);
                if (n < 0) {
                    vScreenPutchar('-');
                    n = -n;
                }
                my_putn((uint32_t)n, 10);
            }
            else if (format[i] == 'x') my_putn(va_arg(args, uint32_t), 16);
            else if (format[i] == 'u') my_putn(va_arg(args, uint32_t), 10);
            else if (format[i] == 's') my_puts(va_arg(args, const char *));
            else if (format[i] == '%') vScreenPutchar('%');
            else if (format[i] == '\0') break;
        }
        else vScreenPutchar(format[i]);
    }
    va_end(args);
}

/*
int sprintf(char *out, const char *format, ...)
{
        va_list args;
        
        va_start( args, format );
        return print( &out, format, args );
}


int snprintf( char *buf, unsigned int count, const char *format, ... )
{
        va_list args;
        
        ( void ) count;
        
        va_start( args, format );
        return print( &buf, format, args );
}
*/
