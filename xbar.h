#ifndef XBAR__H
#define XBAR__H
/* xbar.h - Common data structures. */

#include <assert.h>
#include <stdbool.h>

enum {
    TRIG_FD = 1,
    TRIG_TIME = 2,
};

enum ModStatus {
    ST_OK,
    ST_ERR,
    ST_EOF,
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
    enum ModStatus      (*m_run)(const char **, void *, int);
};
#endif /* ndef XBAR__H */
