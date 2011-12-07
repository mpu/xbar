#include "xbar.h"
#include "mod_say.h"

bool
mod_say_init(struct ModData * pmd)
{
    *pmd = (struct ModData) {
        .count = -1,
        .fds = 0,
    };
    return true;
}

enum ModStatus
mod_say_run(const char ** ret, void * p, int fd)
{
    (void) fd;
    *ret = p;
    return ST_OK;
}
