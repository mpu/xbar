#ifndef XBAR__H
#define XBAR__H
/* xbar.h - Common data structures. */

#include <assert.h>
#include <stdbool.h>

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
    bool                (*m_init)(struct ModData *);
    void                (*m_free)(const char *);
    const char *        (*m_run)(void *, int);
};
#endif /* ndef XBAR__H */
