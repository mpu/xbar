/* xbar 2011 */
#include <stdbool.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

#include <X11/Xlib.h>

#include "xbar.h"

#define LEN(a) (sizeof a / sizeof a[0])

/* MAX_CMDS - Maximum number of consecutive commands in a string given to
 * xputfstr. This can be customized in "config.h".
 */
#define MAX_CMDS 32

enum Pack {
    PACK_LEFT,
    PACK_RIGHT,
};

struct Module {
    const struct ModInfo        mod;
    enum Pack                   pack;
};

#include "config.h"

struct ModRuntime {
    unsigned                    mr_id;
    enum Pack                   mr_pack;
    struct ModData              mr_data;
    const struct ModInfo *      mr_info;
    struct ModRuntime *         mr_nextfd;
    struct ModRuntime *         mr_nexttime;
};

static struct {
    Window              win;
    int                 sn;
    Display *           dsp;
    int                 xfd;
    int                 dspw;
    GC                  gc;
    XFontStruct *       fs;
    unsigned long       fg;
    unsigned long       bg;
} xcnf;

static struct PixelEntry {
    char *              name;
    unsigned long       pixel;
    struct PixelEntry * next;
} * pixcache[256];

// -- Code.
static void complain(const char *);
static void term(int);
static long long utime(void);

static unsigned minit(struct ModRuntime *,
                      struct ModRuntime **, struct ModRuntime **);
static void mloop(void);

static bool xdirty(void);
static void xputstr(int, unsigned long, const char *, int);
static void xputfstr(int *, enum Pack, const char *);
static void xclearbar(void);
static void xdrawbar(const char **, const enum Pack *);
static unsigned long xgetpixel(const char *, unsigned long);
static bool xinit(void);
static void xdeinit(void);

static void
complain(const char * msg)
{
    fputs(msg, stderr);
    fputc('\n', stderr);
}

static void
term(int sig)
{
    (void) sig;
    complain("Killed.");
    xdeinit();
    exit(0);
}

static long long
utime(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

static unsigned
minit(struct ModRuntime * marr,
      struct ModRuntime ** fsttime, struct ModRuntime ** fstfd)
{
    unsigned mid = 0;

    for (unsigned m = 0; m < LEN(modules); m++) {
        if (!modules[m].mod.m_init(&marr->mr_data)) {
            complain("Cannot init module.");
            continue;
        }
        marr->mr_id = mid;
        marr->mr_info = &modules[m].mod;
        marr->mr_pack = modules[m].pack;
        marr->mr_nextfd = marr->mr_nexttime = NULL;
        if (modules[m].mod.m_trigger & TRIG_FD) {
            *fstfd = marr;
            fstfd = &marr->mr_nextfd;
        }
        if (modules[m].mod.m_trigger & TRIG_TIME) {
            *fsttime = marr;
            fsttime = &marr->mr_nexttime;
        }
        marr++;
        mid++;
    }
    *fsttime = *fstfd = NULL;
    return mid;
}

static void
mloop(void)
{
    unsigned nmods;
    struct ModRuntime mods[LEN(modules)];
    struct ModRuntime * fsttime, * fstfd;
    const char * strs[LEN(modules) + 1];
    const char * errstr = "error";
    enum Pack packs[LEN(modules)];
    long long start = utime();
    fd_set modfds;
    int maxfd = xcnf.xfd;
    bool dirty = true;

    /* Initialize modules. */
    nmods = minit(mods, &fsttime, &fstfd);
    strs[nmods] = NULL;
    for (unsigned m = 0; m < nmods; m++) {
        const struct ModInfo * mi = mods[m].mr_info;
        assert(mods[m].mr_id == m); // Simple (programmer's) sanity check.
        if (mi->m_run(&strs[m], mi->m_data, -1) != ST_OK)
            strs[m] = errstr;
        packs[m] = mods[m].mr_pack;
    }
    FD_ZERO(&modfds); FD_SET(xcnf.xfd, &modfds);
    for (struct ModRuntime * m = fstfd; m; m = m->mr_nextfd)
        for (int fd, i = 0; (fd = m->mr_data.md_fds[i]) >= 0; i++) {
            if (fd > maxfd)
                maxfd = fd;
            FD_SET(fd, &modfds);
        }
    maxfd++;

    for (;;) {
        long long time = start;

        for (struct ModRuntime * m = fsttime; m; m = m->mr_nexttime)
            if (m->mr_data.md_count == 0) {
                const struct ModInfo * mi = m->mr_info;
                if (mi->m_run(&strs[m->mr_id], mi->m_data, -1) != ST_OK)
                    strs[m->mr_id] = errstr;
                dirty = true;
            } else if (m->mr_data.md_count > 0)
                m->mr_data.md_count--;

        do {
            unsigned wait = refresh_delay + (start - time);
            fd_set fds = modfds;
            int ret;

            fds = modfds;
            ret = select(maxfd, &fds, 0, 0,
                         &(struct timeval){ .tv_usec = wait });

            if (ret < 0) {
                if (errno == EAGAIN || errno == EINTR)
                    continue;
                complain("Select failed.");
                return;
            }

            for (struct ModRuntime * m = fstfd; m; m = m->mr_nextfd)
                for (int fd, i = 0; (fd = m->mr_data.md_fds[i]) >= 0; i++)
                    if (FD_ISSET(fd, &fds)) {
                        const struct ModInfo * mi = m->mr_info;
                        enum ModStatus st;
                        st = mi->m_run(&strs[m->mr_id], mi->m_data, fd);
                        switch (st) {
                        case ST_OK:
                            break;
                        case ST_ERR:
                            strs[m->mr_id] = errstr;
                            break;
                        case ST_EOF:
                            close(fd);
                            FD_CLR(fd, &modfds);
                            break;
                        }
                        dirty = true;
                    }
            if (dirty ||
                (ret > 0 && FD_ISSET(xcnf.xfd, &fds) && xdirty())) {
                xdrawbar(strs, packs);
                dirty = false;
            }
        } while ((time = utime()) < start + refresh_delay);
        start = time;
    }
}

static bool
xdirty(void)
{
    XEvent xev;
    bool ret = false;

    while (XPending(xcnf.dsp) > 0) {
        XNextEvent(xcnf.dsp, &xev);
        if (xev.type == Expose && xev.xexpose.y < bar_height)
            ret = true;
    }
    return ret;
}

static void
xputstr(int x, unsigned long fg, const char * str, int len)
{
    const int y = xcnf.fs->max_bounds.ascent + 1;

    assert(len > 0);
    XSetForeground(xcnf.dsp, xcnf.gc, fg);
    XDrawString(xcnf.dsp, xcnf.win, xcnf.gc, x , y, str, len);
}

static void
xputfstr(int * x, enum Pack pack, const char * str)
{
    struct DrawData {
        unsigned long   fg;
        int             len;
        const char *    str;
    } strings[MAX_CMDS];
    unsigned sid = 0;
    unsigned long fg = xcnf.fg;
    int len = 0;
    const char * p = str;

    while (*p) {
        char * lparen, * rparen;

        if (*p != '^') {
            p++, len++;
            continue;
        }
        if ((lparen = strchr(p, '(')) == NULL ||
            (rparen = strchr(lparen + 1, ')')) == NULL) {
            complain("Syntax error in display string.");
            p++, len++;
            continue;
        }
        if (sid >= LEN(strings))
            break;
        if (len)
            strings[sid++] = (struct DrawData) {
                .fg = fg,
                .len = len,
                .str = str
            };
        if (strncmp(p + 1, "fg(", 3) == 0) {
            const size_t fg_len = rparen - lparen - 1;
            char fg_str[64];

            if (fg_len >= 64) {
                complain("Color string too long in 'fg' command.");
                goto nextchunk;
            }
            if (fg_len == 0) {
                fg = xcnf.fg;
                goto nextchunk;
            }
            strncpy(fg_str, lparen + 1, fg_len);
            fg_str[fg_len] = 0;
            fg = xgetpixel(fg_str, xcnf.fg);
        } else
            complain("Unknown command in display string.");
nextchunk:
        str = p = rparen + 1;
        len = 0;
    }
    if (len && sid < LEN(strings))
        strings[sid++] = (struct DrawData) {
            .fg = fg,
            .len = len,
            .str = str
        };
    switch (pack) {
    case PACK_LEFT:
        for (unsigned i = 0; i < sid; i++) {
            const int wid =
                XTextWidth(xcnf.fs, strings[i].str, strings[i].len);
            xputstr(*x, strings[i].fg, strings[i].str, strings[i].len);
            *x += wid;
        }
        break;
    case PACK_RIGHT:
        for (unsigned j = 0, i = sid - 1; j < sid; j++, i--) {
            const int wid =
                XTextWidth(xcnf.fs, strings[i].str, strings[i].len);
            *x -= wid;
            xputstr(*x, strings[i].fg, strings[i].str, strings[i].len);
        }
        break;
    }
}

static void
xclearbar(void)
{
    XSetForeground(xcnf.dsp, xcnf.gc, xcnf.bg);
    XFillRectangle(xcnf.dsp, xcnf.win, xcnf.gc,
                   0, 0, xcnf.dspw, bar_height);
    XSetForeground(xcnf.dsp, xcnf.gc, xcnf.fg);
}

static void
xdrawbar(const char ** strs, const enum Pack * packs)
{
    bool first[2] = { true, true };
    int x[2] = { [PACK_RIGHT] = xcnf.dspw };

    xclearbar();
    for (; *strs; packs++, strs++) {
        assert(*packs < 2);
        if (!first[*packs])
            xputfstr(&x[*packs], *packs, sep);
        else
            first[*packs] = false;
        xputfstr(&x[*packs], *packs, *strs);
    }
    XFlush(xcnf.dsp);
}

static unsigned long
xgetpixel(const char * name, unsigned long def)
{
    struct PixelEntry ** ppe, * pe;
    unsigned long pixel;
    unsigned hash = 0;
    XColor xc;

    for (const char * p = name; *p; p++)
        hash = hash * 19 + *p;
    hash &= 255;

    for (ppe = &pixcache[hash]; (pe = *ppe); ppe = &(*ppe)->next)
        if (strcmp(pe->name, name) == 0)
            return pe->pixel;

    if (!XAllocNamedColor(xcnf.dsp, DefaultColormap(xcnf.dsp, xcnf.sn),
                          name, &xc, &xc)) {
        complain("Cannot allocate color.");
        pixel = def;
    } else
        pixel = xc.pixel;

    if ((*ppe = pe = malloc(sizeof(struct PixelEntry))) &&
        (pe->name = malloc(strlen(name) + 1))) {
        strcpy(pe->name, name);
        pe->pixel = pixel;
        pe->next = NULL;
    } else if (pe && pe->name == NULL) {
        free(pe);
        *ppe = NULL;
    }
    return pixel;
}

static bool
xinit(void)
{
    XGCValues gcv;
    XWindowAttributes wattr;

    if ((xcnf.dsp = XOpenDisplay(NULL)) == NULL) {
        complain("Cannot open display.");
        return false;
    }
    xcnf.sn = DefaultScreen(xcnf.dsp);
    xcnf.xfd = ConnectionNumber(xcnf.dsp);
    xcnf.win = RootWindow(xcnf.dsp, xcnf.sn);
    XSelectInput(xcnf.dsp, xcnf.win, ExposureMask);
    if (XGetWindowAttributes(xcnf.dsp, xcnf.win, &wattr) == 0) {
        complain("Cannot retreive the size of the root window.");
        xcnf.dspw = 800;
    } else
        xcnf.dspw = wattr.width;
    if ((xcnf.fs = XLoadQueryFont(xcnf.dsp, font)) == NULL) {
        XCloseDisplay(xcnf.dsp);
        complain("Cannot load font.");
        return false;
    }
    xcnf.fg = xgetpixel(fgcolor, WhitePixel(xcnf.dsp, xcnf.sn));
    xcnf.bg = xgetpixel(bgcolor, BlackPixel(xcnf.dsp, xcnf.sn));
    gcv = (XGCValues) {
        .foreground = xcnf.fg,
        .background = xcnf.bg,
        .font = xcnf.fs->fid,
    };
    xcnf.gc = XCreateGC(xcnf.dsp, RootWindow(xcnf.dsp, xcnf.sn),
                        GCForeground | GCBackground | GCFont, &gcv);
    return true;
}

static void
xdeinit(void)
{
    for (unsigned i = 0; i < LEN(pixcache); i++) {
        struct PixelEntry * pn, * pe = pixcache[i];
        while (pe) {
            pn = pe->next;
            free(pe->name);
            free(pe);
            pe = pn;
        }
    }
    XFreeGC(xcnf.dsp, xcnf.gc);
    XFreeFont(xcnf.dsp, xcnf.fs);
    XCloseDisplay(xcnf.dsp);
}

int
main(void)
{
    if (!xinit())
        return 1;
    signal(SIGTERM, term);
    signal(SIGINT, term);
    mloop();
    xdeinit();
    return 0;
}
