#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include "xbar.h"
#include "mod_cpu.h"

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static unsigned load;

struct CpuLoad {
    unsigned cpu_user;
    unsigned cpu_sys;
    unsigned cpu_tot;
};

static bool mod_cpu_fetch(struct CpuLoad *);
static void * mod_cpu_probe(void *);

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

static void *
mod_cpu_probe(void * p)
{
    struct CpuLoad load0, load1;

    (void) p;
    if (!mod_cpu_fetch(&load0))
        goto err_fetch;
    for (;;) {
        sleep(1);
        if (!mod_cpu_fetch(&load1))
            goto err_fetch;
        if (pthread_mutex_lock(&lock))
            continue;
        load = 100 * (load1.cpu_user + load1.cpu_sys
                     -load0.cpu_user - load0.cpu_sys) /
                     (load1.cpu_tot - load0.cpu_tot);
        /* printf("user: %d\n", load1.cpu_user); */
        pthread_mutex_unlock(&lock);
        load0 = load1;
    }
    return 0;

err_fetch:
    fputs("Cannot fetch CPU info.\n", stderr);
    return 0;
}

bool
mod_cpu_init(struct ModData * pmd)
{
    pthread_t pth;
    struct ModData md = {
        .md_count = 3,
        .md_fds = 0,
    };

    if (pthread_create(&pth, NULL, mod_cpu_probe, NULL)) {
        perror("mod_cpu_init");
        return false;
    }
    *pmd = md;
    return true;
}

const char *
mod_cpu_run(void * p, int fd)
{
    static const char err[] = "mod_cpu: err";
    static char buf[32];

    (void) fd;
    if (pthread_mutex_lock(&lock))
        return err;
    snprintf(buf, sizeof buf, (const char *)p, load);
    pthread_mutex_unlock(&lock);
    return buf;
}
