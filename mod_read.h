#ifndef MOD_READ__H
#define MOD_READ__H

extern bool mod_read_init(struct ModData *);
extern enum ModStatus mod_read_run(const char **, void *, int);

#define MOD_READ()                      \
    (struct ModInfo) {                  \
        .m_name = "mod_read",           \
        .m_period = -1,                 \
        .m_trigger = TRIG_FD,           \
        .m_data = NULL,                 \
        .m_init = mod_read_init,        \
        .m_run = mod_read_run,          \
    }

#endif /* MOD_READ__H */
