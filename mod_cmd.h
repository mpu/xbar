#ifndef MOD_CMD__H
#define MOD_CMD__H

#include "xbar.h"

extern bool mod_cmd_init(struct ModData *);
extern const char * mod_cmd_run(void *, int);

#define MOD_CMD(cmd, period)    \
    (struct ModInfo) {          \
        .m_name = "mod_cmd",    \
        .m_period = period,     \
        .m_trigger = TRIG_TIME, \
        .m_data = (void *)cmd,  \
        .m_init = mod_cmd_init, \
        .m_run = mod_cmd_run,   \
        .m_free = NULL,         \
    }

#endif /* ndef MOD_CMD__H */
