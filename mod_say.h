#ifndef MOD_SAY__H
#define MOD_SAY__H

#include "xbar.h"

extern bool mod_say_init(struct ModData *);
extern enum ModStatus mod_say_run(const char **, void *, int);

#define MOD_SAY(s)              \
    (struct ModInfo) {          \
        .name = "mod_say",      \
        .period = -1,           \
        .trigger = 0,           \
        .data = (void *)s,      \
        .init = mod_say_init,   \
        .run = mod_say_run,     \
    }

#endif /* ndef MOD_SAY__H */
