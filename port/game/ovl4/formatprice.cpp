// OVL4 8001fcdc-80020198 (formatprice): Prices, power, speeds and days as wide text.
#include "common.h"

extern "C" {
extern char (&sB_Nhp)[];
extern char (&D_801C3108)[];
extern char (&s_FmtDay)[];

// Writes price in decimal with a comma every three digits, as Credits_FormatCommas.
void FormatPrice(unsigned short *buf, unsigned int price)
{
    char digits[64];
    int div = 10000000;
    int started = 0;
    int n;
    int group;
    int c;

    if (price == 0) {
        buf[0] = '0';
        buf[1] = 0;
        return;
    }
    n = 0;
    for (;;) {
        int d = price / div;
        int nonzero = d != 0;

        if (nonzero | started) {
            digits[n] = d + '0';
            started = 1;
            n++;
            price -= d * div;
        }
        if (div == 1) {
            break;
        }
        div /= 10;
    }
    digits[n] = 0;
    group = n % 3;
    if (group == 0) {
        group = 3;
    }
    *buf = (signed char)digits[0];
    c = digits[1];
    n = 0;
    goto test;
loop:
    if (--group == 0) {
        *buf++ = ',';
        group = 3;
    }
    n++;
    *buf = (signed char)digits[n];
    c = (signed char)digits[n + 1];
test:
    buf++;
    if (c != 0) {
        goto loop;
    }
    *buf = 0;
}

// Writes the price hi * 100000000 + lo, as FormatPrice.
// Doesn't match: the second loop's digit, its 1 and its 0x66666667 get other
// registers (the game gives the digit the first loop's a3).
void FormatPriceHiLo(unsigned short *buf, unsigned int hi, unsigned int lo)
{
    char digits[128];
    int div = 10000000;
    int started = 0;
    int n;
    int group;
    int c;

    if ((hi == 0) & (lo == 0)) {
        buf[0] = '0';
        buf[1] = 0;
        return;
    }
    n = 0;
    if (hi == 0) {
        FormatPrice(buf, lo);
        return;
    }
    for (;;) {
        int d = hi / div;
        int nonzero = d != 0;

        if (nonzero | started) {
            digits[n] = d + '0';
            started = 1;
            n++;
            hi -= d * div;
        }
        if (div == 1) {
            break;
        }
        div /= 10;
    }
    div = 10000000;
    for (;;) {
        int d = lo / div;

        digits[n++] = d + '0';
        lo -= d * div;
        if (div == 1) {
            break;
        }
        div /= 10;
    }
    digits[n] = 0;
    group = n % 3;
    if (group == 0) {
        group = 3;
    }
    *buf = (signed char)digits[0];
    c = (signed char)digits[1];
    n = 0;
    goto test;
loop:
    if (--group == 0) {
        *buf++ = ',';
        group = 3;
    }
    n++;
    *buf = (signed char)digits[n];
    c = (signed char)digits[n + 1];
test:
    buf++;
    if (c != 0) {
        goto loop;
    }
    *buf = 0;
}

// Writes the power hp ("%dhp"-style, sB_Nhp).
void FormatHpW(unsigned short *buf, int hp)
{
    char text[64];
    int i;

    sprintf(text, sB_Nhp, hp);
    for (i = 0; *buf++ = (signed char)text[i], (signed char)text[i] != 0; i++)
        ;
}

// Writes the speed v / 100 with its decimals (D_801C3108).
void CarSpec_FormatSpeed(unsigned short *buf, int v)
{
    char text[64];
    int i;

    sprintf(text, D_801C3108, v / 100, v % 100);
    for (i = 0; *buf++ = (signed char)text[i], (signed char)text[i] != 0; i++)
        ;
}

// Writes the day count day (s_FmtDay).
void FormatDayW(unsigned short *buf, int day)
{
    char text[64];
    int i;

    sprintf(text, s_FmtDay, day);
    for (i = 0; *buf++ = (signed char)text[i], (signed char)text[i] != 0; i++)
        ;
}
}
