/* xbar.h - Common data structures. */
#ifndef XBAR__H
#define XBAR__H

enum {
    TRIG_FD = 1,
    TRIG_TIME = 2,
};

struct ModData {
    int         md_count;
    int *       md_fds;
};

struct ModInfo {
    const char *        m_name;
    int                 m_period;
    unsigned int        m_trigger;
    void *              m_data;
    struct ModData *    (*m_init)(void);
    void                (*m_free)(const char *);
    char *              (*m_run)(void *, int);
};
#endif /* ndef XBAR__H */
