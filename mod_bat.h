#ifndef MOD_BAT__H
#define MOD_BAT__H

extern bool mod_bat_init(struct ModData *);
extern const char * mod_bat_run(void *, int);

#define MOD_BAT(bat, fmt)                       \
    (struct ModInfo) {                          \
        .m_name = "mod_bat",                    \
        .m_period = 20,                         \
        .m_trigger = TRIG_TIME,                 \
        .m_data = (const char *[]){ bat, fmt }, \
        .m_init = mod_bat_init,                 \
        .m_run = mod_bat_run,                   \
        .m_free = NULL,                         \
    }

#endif /* ndef MOD_BAT__H */
