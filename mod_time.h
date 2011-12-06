#ifndef MOD_TIME__H
#define MOD_TIME__H

#include "xbar.h"

extern bool mod_time_init(struct ModData *);
extern enum ModStatus mod_time_run(const char **, void *, int);

#define MOD_TIME()                      \
    (struct ModInfo) {                  \
        .m_name = "mod_time",           \
        .m_period = 5,                  \
        .m_trigger = TRIG_TIME,         \
        .m_data = 0,                    \
        .m_init = mod_time_init,        \
        .m_run = mod_time_run,          \
    }

#endif /* ndef MOD_TIME__H */
