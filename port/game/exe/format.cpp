// Resident code 80068734-80068e2c (format): lap times, time differences, speeds and
// credit amounts as text.
#include "common.h"

extern "C" {
extern char (&D_80091AAC)[]; // shown for a missing time
extern char (&D_8008FA68)[];

// Writes ms as m:ss.mmm, or h:mm:ss.t from an hour on; returns the text length.
int Time_FormatMs(char *buf, unsigned int ms)
{
    int len = 5;
    int min;
    int sec;
    int part; // the milliseconds, or the hours and then the tenths
    int d;

    if (ms == 0xffffffff) {
        strcpy(buf, D_80091AAC);
    }
    if (ms < 3600000) {
        min = ms / 60000;
        ms -= min * 60000;
        len = 4;
        d = min / 10;
        if (d != 0) {
            *buf++ = d + '0';
            len = 5;
        }
        *buf++ = min - d * 10 + '0';
        sec = ms / 1000;
        part = ms - sec * 1000;
        *buf++ = ':';
        d = sec / 10;
        *buf++ = d + '0';
        *buf++ = sec - d * 10 + '0';
        d = part / 100;
        part -= d * 100;
        *buf++ = '.';
        *buf++ = d + '0';
        d = part / 10;
        *buf++ = d + '0';
        *buf = part - d * 10 + '0';
        buf[1] = 0;
    } else if (ms < 360000000) {
        len = 7;
        if (ms >= 36000000) {
            len = 8;
            part = ms / 36000000;
            ms -= part * 36000000;
            *buf++ = part + '0';
        }
        part = ms / 3600000;
        ms -= part * 3600000;
        min = ms / 60000;
        ms -= min * 60000;
        *buf++ = part + '0';
        *buf++ = ':';
        d = min / 10;
        *buf++ = d + '0';
        *buf++ = min - d * 10 + '0';
        *buf++ = ':';
        sec = ms / 1000;
        ms -= sec * 1000;
        d = sec / 10;
        *buf++ = d + '0';
        *buf++ = sec - d * 10 + '0';
        *buf++ = '.';
        buf[1] = 0;
        part = ms / 100;
        *buf = part + '0';
    } else {
        strcpy(buf, D_80091AAC);
    }
    return len;
}

// Writes the difference between two times as a sign then s.mmm, or as
// Time_FormatMs text from a minute on.
void Time_FormatDiff(char *buf, unsigned int a, unsigned int b)
{
    unsigned int diff;
    int ms;
    int sec;
    int d;
    int rest;

    if ((a == 0xffffffff) | (b == 0xffffffff)) {
        strcpy(buf, D_80091AAC);
        return;
    }
    diff = a - b;
    *buf = '-';
    if (a == b) {
        diff = 0;
        *(signed char *)buf = 0xb1; // plus-minus sign
    } else if (a < b) {
        diff = b - a;
        *buf = '+';
    }
    ms = diff;
    buf++;
    if (diff > 59999) {
        Time_FormatMs(buf, ms);
        return;
    }
    sec = diff / 1000;
    ms -= sec * 1000;
    d = sec / 10;
    if (d != 0) {
        *buf++ = d + '0';
    }
    *buf++ = sec - d * 10 + '0';
    *buf++ = '.';
    d = ms / 100;
    rest = ms - d * 100;
    *buf++ = d + '0';
    d = rest / 10;
    *buf++ = d + '0';
    *buf = rest - d * 10 + '0';
    buf[1] = 0;
}

void Speed_Format(char *buf, unsigned int v)
{
    if (99999 < v) {
        v = 99999;
    }
    sprintf(buf, D_8008FA68, v / 100, v % 100);
}

// Writes v in decimal with a comma every three digits.
void Credits_FormatCommas(char *buf, unsigned int v)
{
    char digits[64];
    int div = 10000000;
    int started = 0;
    int n;
    int group;
    int c;

    if (v == 0) {
        buf[0] = '0';
        buf[1] = 0;
        return;
    }
    n = 0;
    for (;;) {
        int d = v / div;
        int nonzero = d != 0;

        if (nonzero | started) {
            digits[n] = d + '0';
            started = 1;
            n++;
            v -= d * div;
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
    *buf = digits[0];
    c = digits[1];
    n = 0;
    goto test;
loop:
    if (--group == 0) {
        *buf++ = ',';
        group = 3;
    }
    n++;
    *buf = digits[n];
    c = (signed char)digits[n + 1]; // lb, where the first test reads lbu
test:
    buf++;
    if (c != 0) {
        goto loop;
    }
    *buf = 0;
}
}
