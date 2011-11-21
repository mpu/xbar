#include "xbar.h"
#include "mod_say.h"

struct ModData *
mod_say_init(void)
{
    static struct ModData md = {
        .md_count = -1,
        .md_fds = 0,
    };
    return &md;
}

char *
mod_say_run(void * p, int fd)
{
    (void) fd;
    return (char *)p;
}
