#ifndef MOD_READ__H
#define MOD_READ__H

extern bool mod_read_init(struct ModData *);
extern enum ModStatus mod_read_run(const char **, void *, int);

#define MOD_READ()              \
    (struct ModInfo) {          \
        .name = "mod_read",     \
        .period = -1,           \
        .trigger = TRIG_FD,     \
        .data = NULL,           \
        .init = mod_read_init,  \
        .run = mod_read_run,    \
    }

#endif /* MOD_READ__H */
