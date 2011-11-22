#include "xbar.h"
#include "mod_say.h"

bool
mod_say_init(struct ModData * pmd)
{
    struct ModData md = {
        .md_count = -1,
        .md_fds = 0,
    };
    *pmd = md;
    return true;
}

char *
mod_say_run(void * p, int fd)
{
    (void) fd;
    return (char *)p;
}
