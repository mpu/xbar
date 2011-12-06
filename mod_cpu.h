#ifndef MOD_CPU__H
#define MOD_CPU__H

#include "xbar.h"

extern bool mod_cpu_init(struct ModData *);
extern enum ModStatus mod_cpu_run(const char **, void *, int);

#define MOD_CPU(fmt)            \
    (struct ModInfo) {          \
        .m_name = "mod_cpu",    \
        .m_period = 0,          \
        .m_trigger = TRIG_TIME, \
        .m_data = (void *)fmt,  \
        .m_init = mod_cpu_init, \
        .m_run = mod_cpu_run,   \
    }

#endif /* ndef MOD_CPU__H */
