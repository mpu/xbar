#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "xbar.h"
#include "mod_bat.h"

bool
mod_bat_init(struct ModData * pmd)
{
    *pmd = (struct ModData) {
        .count = 20,
        .fds = 0,
    };
    return true;
}

enum ModStatus
mod_bat_run(const char ** ret, void * p, int fd)
{
    static char buf[32];
    const struct BatData * data = p;
    const char prefix[] = "/sys/class/power_supply/";
    const size_t prefix_len = sizeof prefix + strlen(data->bat) - 1;
    char * fname;
    enum ModStatus status = ST_OK;
    FILE * fnow, * ffull;
    unsigned chnow, chfull;

    (void) fd;
    if ((fname = malloc(prefix_len + 1 + 12)) == NULL)
        return ST_ERR;
    strcpy(fname, prefix);
    strcat(fname, data->bat);
    strcpy(&fname[prefix_len], "/charge_now");
    fnow = fopen(fname, "r");
    strcpy(&fname[prefix_len], "/charge_full");
    ffull = fopen(fname, "r");
    if (!fnow || !ffull ||
        fscanf(fnow, "%u", &chnow) < 1 ||
        fscanf(ffull, "%u", &chfull) < 1 ||
        chfull == 0) {
        status = ST_ERR;
        goto eio;
    }
    snprintf(buf, sizeof buf, data->fmt, 100 * chnow / chfull);
    *ret = buf;

eio:
    free(fname);
    if (fnow)
        fclose(fnow);
    if (ffull)
        fclose(ffull);
    return status;
}
