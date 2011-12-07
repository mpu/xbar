#ifndef MOD_CPU__H
#define MOD_CPU__H

#include "xbar.h"

extern bool mod_cpu_init(struct ModData *);
extern enum ModStatus mod_cpu_run(const char **, void *, int);

#define MOD_CPU(fmt)            \
    (struct ModInfo) {          \
        .name = "mod_cpu",      \
        .period = 0,            \
        .trigger = TRIG_TIME,   \
        .data = (void *)fmt,    \
        .init = mod_cpu_init,   \
        .run = mod_cpu_run,     \
    }

#endif /* ndef MOD_CPU__H */
