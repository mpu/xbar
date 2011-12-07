#include <stdio.h>

#include "xbar.h"
#include "mod_cpu.h"

struct CpuInfo {
    unsigned cpu_user;
    unsigned cpu_sys;
    unsigned cpu_tot;
};

static struct CpuInfo last_info;
static unsigned last_load;

static bool mod_cpu_fetch(struct CpuInfo *);

static bool
mod_cpu_fetch(struct CpuInfo * load)
{
    int read;
    unsigned nice, idle;
    FILE * f = fopen("/proc/stat", "r");

    if (f == NULL)
        return false;
    read = fscanf(f, "cpu %u %u %u %u",
                  &load->cpu_user, &nice, &load->cpu_sys, &idle);
    fclose(f);
    if (read != 4)
        return false;
    load->cpu_tot = load->cpu_user + load->cpu_sys + nice + idle;
    return true;
}

bool
mod_cpu_init(struct ModData * pmd)
{
    *pmd = (struct ModData) {
        .count = 0,
        .fds = 0,
    };
    return true;
}

enum ModStatus
mod_cpu_run(const char ** ret, void * p, int fd)
{
    static const char up[] = "Updating...";
    static char buf[32];
    struct CpuInfo info;

    (void) fd;
    if (last_info.cpu_tot == 0) {
        mod_cpu_fetch(&last_info);
        *ret = up;
        return ST_OK;
    }
    if (!mod_cpu_fetch(&info)) {
        fputs("Cannot fetch cpu info.\n", stderr);
        return ST_ERR;
    }
    if (info.cpu_tot != last_info.cpu_tot)
        last_load = 100 * (info.cpu_user      + info.cpu_sys
                          -last_info.cpu_user - last_info.cpu_sys) /
                          (info.cpu_tot - last_info.cpu_tot);
    snprintf(buf, sizeof buf, (const char *)p, last_load);
    last_info = info;
    *ret = buf;
    return ST_OK;
}
