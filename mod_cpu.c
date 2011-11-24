#include <stdio.h>

#include "xbar.h"
#include "mod_cpu.h"

struct CpuLoad {
    unsigned cpu_user;
    unsigned cpu_sys;
    unsigned cpu_tot;
};

static struct CpuLoad last_load;

static bool mod_cpu_fetch(struct CpuLoad *);

static bool
mod_cpu_fetch(struct CpuLoad * load)
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
    struct ModData md = {
        .md_count = 0,
        .md_fds = 0,
    };
    *pmd = md;
    return true;
}

const char *
mod_cpu_run(void * p, int fd)
{
    static const char up[] = "Updating...";
    static char buf[32];
    unsigned load;
    struct CpuLoad ld;

    (void) fd;
    if (last_load.cpu_tot == 0) {
        mod_cpu_fetch(&last_load);
        return up;
    }
    if (!mod_cpu_fetch(&ld)) {
        fputs("Cannot fetch cpu info.\n", stderr);
        return up;
    }
    if (ld.cpu_tot == last_load.cpu_tot)
        return up;
    load = 100 * (ld.cpu_user        + ld.cpu_sys
                 -last_load.cpu_user - last_load.cpu_sys) /
                 (ld.cpu_tot - last_load.cpu_tot);
    snprintf(buf, sizeof buf, (const char *)p, load);
    last_load = ld;
    return buf;
}
