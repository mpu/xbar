#ifndef MOD_BAT__H
#define MOD_BAT__H

#include "xbar.h"

struct BatData {
    const char * bat;
    const char * fmt;
};

extern bool mod_bat_init(struct ModData *);
extern enum ModStatus mod_bat_run(const char **, void *, int);

#define MOD_BAT(bat, fmt)                       \
    (struct ModInfo) {                          \
        .m_name = "mod_bat",                    \
        .m_period = 20,                         \
        .m_trigger = TRIG_TIME,                 \
        .m_data = &(struct BatData){ bat, fmt },\
        .m_init = mod_bat_init,                 \
        .m_run = mod_bat_run,                   \
    }

#endif /* ndef MOD_BAT__H */
