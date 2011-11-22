#include <stdio.h>
#include <string.h>
#include <time.h>

#include "xbar.h"
#include "mod_time.h"

static char * mod_time_time(const struct tm *);

static char *
mod_time_time(const struct tm * tm)
{
    static char buf[32];
    static const char * dow[] = {
        "sun", "mon", "tue", "wed", "thu", "fri", "sat"
    };
    static const char * mon[] = {
        "jan", "feb", "mar", "apr", "may", "jun",
        "jul", "aug", "sep", "oct", "nov", "dec"
    };

    assert(tm->tm_wday < 7 && tm->tm_mon < 12 &&
           tm->tm_mday < 32 && tm->tm_hour < 60 && tm->tm_min < 60 &&
           tm->tm_year < 3000);
    sprintf(buf, "%s %s %02d %02d:%02d %d",
            dow[tm->tm_wday], mon[tm->tm_mon],
            tm->tm_mday, tm->tm_hour, tm->tm_min,
            tm->tm_year + 1900);
    return buf;
}

bool
mod_time_init(struct ModData * pmd)
{
    struct ModData md = {
        .md_count = 0,
        .md_fds = 0,
    };
    *pmd = md;
    return true;
}

char *
mod_time_run(void * p, int fd)
{
    const time_t tm = time(NULL);

    (void) p;
    (void) fd;
    return mod_time_time(localtime(&tm));
}
