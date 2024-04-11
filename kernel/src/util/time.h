#ifndef _TIME_H
#define _TIME_H

#include <stdint.h>
typedef uint64_t time_t;

extern char tzname[2][3];
extern int daylight;
extern long timezone;
 
struct tm {
    int tm_sec;         /* seconds */
    int tm_min;         /* minutes */
    int tm_hour;        /* hours */
    int tm_mday;        /* day of the month */
    int tm_mon;         /* month */
    int tm_year;        /* year */
    int tm_wday;        /* day of the week */
    int tm_yday;        /* day in the year */
    int tm_isdst;       /* daylight saving time */
};

uint64_t strftime(char *s, uint64_t max, const char *format, const struct tm *tm);
time_t time(time_t *tloc);
struct tm *localtime(const time_t *timep);
void tzset(void);
#endif