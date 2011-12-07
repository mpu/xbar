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
        .name = "mod_bat",                      \
        .period = 20,                           \
        .trigger = TRIG_TIME,                   \
        .data = &(struct BatData){ bat, fmt },  \
        .init = mod_bat_init,                   \
        .run = mod_bat_run,                     \
    }

#endif /* ndef MOD_BAT__H */
