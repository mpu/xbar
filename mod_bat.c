#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "xbar.h"
#include "mod_bat.h"

bool
mod_bat_init(struct ModData * pmd)
{
    struct ModData md = {
        .md_count = 20,
        .md_fds = 0,
    };
    *pmd = md;
    return true;
}

const char *
mod_bat_run(void * p, int fd)
{
    static char buf[32];
    static const char * err = "mod_bat: Error";
    const char ** data = p;
    const char prefix[] = "/sys/class/power_supply/";
    const size_t prefix_len = sizeof prefix + strlen(data[0]) - 1;
    char * fname;
    const char * ret = buf;
    FILE * fnow, * ffull;
    unsigned chnow, chfull;

    (void) fd;
    if ((fname = malloc(prefix_len + 1 + 12)) == NULL)
        return err;
    strcpy(fname, prefix);
    strcat(fname, data[0]);
    strcpy(&fname[prefix_len], "/charge_now");
    fnow = fopen(fname, "r");
    strcpy(&fname[prefix_len], "/charge_full");
    ffull = fopen(fname, "r");
    if (!fnow || !ffull ||
        fscanf(fnow, "%u", &chnow) < 1 ||
        fscanf(ffull, "%u", &chfull) < 1 ||
        chfull == 0) {
        ret = err;
        goto eio;
    }
    snprintf(buf, sizeof buf, data[1], 100 * chnow / chfull);

eio:
    free(fname);
    if (fnow)
        fclose(fnow);
    if (ffull)
        fclose(ffull);
    return ret;
}
