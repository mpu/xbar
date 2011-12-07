#ifndef MOD_TIME__H
#define MOD_TIME__H

#include "xbar.h"

extern bool mod_time_init(struct ModData *);
extern enum ModStatus mod_time_run(const char **, void *, int);

#define MOD_TIME()              \
    (struct ModInfo) {          \
        .name = "mod_time",     \
        .period = 5,            \
        .trigger = TRIG_TIME,   \
        .data = 0,              \
        .init = mod_time_init,  \
        .run = mod_time_run,    \
    }

#endif /* ndef MOD_TIME__H */
