#ifndef MOD_CMD__H
#define MOD_CMD__H

#include "xbar.h"

extern bool mod_cmd_init(struct ModData *);
extern enum ModStatus mod_cmd_run(const char **, void *, int);

#define MOD_CMD(cmd, p)         \
    (struct ModInfo) {          \
        .name = "mod_cmd",      \
        .period = p,            \
        .trigger = TRIG_TIME,   \
        .data = (void *)cmd,    \
        .init = mod_cmd_init,   \
        .run = mod_cmd_run,     \
    }

#endif /* ndef MOD_CMD__H */
