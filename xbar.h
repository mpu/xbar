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
    int         count;
    int *       fds;
};

struct ModInfo {
    const char *        name;
    int                 period;
    unsigned int        trigger;
    void *              data;
    bool                (*init)(struct ModData *);
    enum ModStatus      (*run)(const char **, void *, int);
};
#endif /* ndef XBAR__H */
