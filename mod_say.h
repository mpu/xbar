#ifndef MOD_SAY__H
#define MOD_SAY__H

#include "xbar.h"

extern struct ModData * mod_say_init(void);
extern char * mod_say_run(void *, int);

#define MOD_SAY(s)              \
    (struct ModInfo) {          \
        .m_name = "mod_say",    \
        .m_period = -1,         \
        .m_trigger = 0,         \
        .m_data = (void *)s,    \
        .m_init = mod_say_init, \
        .m_free = NULL,         \
        .m_run = mod_say_run,   \
    }

#endif /* ndef MOD_SAY__H */
