/* xbar 2011 */
#include <stdbool.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <sys/select.h>
#include <sys/time.h>

#include <X11/Xlib.h>

#include "xbar.h"

#define LEN(a) (sizeof a / sizeof a[0])

/* MAX_CMDS - Maximum number of consecutive commands in a string given to
 * xputfstr. This can be customized in "config.h".
 */
#define MAX_CMDS 64

enum Pack {
    PACK_LEFT,
    PACK_RIGHT,
};

struct Module {
    const struct ModInfo        mod;
    enum Pack                   pack;
};

#include "config.h"

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

static void
mloop(void)
{
    unsigned m, nmod = 0;
    struct ModData md[LEN(modules)];
    const struct ModInfo * mods[LEN(modules)];
    enum Pack packs[LEN(modules)];
    const char * strs[LEN(modules) + 1] = { 0 };
    long long start = utime();
    fd_set read_fds;
    int max_read_fd = 0;
    bool dirty = true;

    FD_ZERO(&read_fds);
    for (m = 0; m < LEN(modules); m++) {
        if (!modules[m].mod.m_init(&md[nmod])) {
            complain("Cannot start module.");
            continue;
        }
        strs[nmod] = modules[m].mod.m_run(modules[m].mod.m_data, -1);
        mods[nmod] = &modules[m].mod;
        packs[nmod] = modules[m].pack;
        if (modules[m].mod.m_trigger & TRIG_FD) {
            int i, fd;
            for (i = 0; (fd = md[nmod].md_fds[i]); i++) {
                if (fd > max_read_fd)
                    max_read_fd = fd;
                FD_SET(fd, &read_fds);
            }
        }
        nmod++;
    }

    for (;;) {
        long long time = start;
        fd_set fds;
        int ret;

        do {
            fds = read_fds;
            ret = select(max_read_fd + 1, &fds, 0, 0,
                         &(struct timeval){ .tv_sec = 0 });
        } while (ret < 0 && (errno == EAGAIN || errno == EINTR));

        if (ret < 0) {
            complain("Select failed.");
            return;
        }

        for (m = 0; m < nmod; m++) {
            int i, fd;
            if (mods[m]->m_trigger & TRIG_TIME) {
                if (md[m].md_count == 0) {
                    if (mods[m]->m_free && strs[m])
                        mods[m]->m_free(strs[m]);
                    strs[m] = mods[m]->m_run(mods[m]->m_data, -1);
                    dirty = true;
                    md[m].md_count = mods[m]->m_period;
                } else if (md[m].md_count > 0)
                    md[m].md_count--;
            }
            if (mods[m]->m_trigger & TRIG_FD)
                for (i = 0; (fd = md[m].md_fds[i]); i++) {
                    if (FD_ISSET(fd, &fds)) {
                        if (mods[m]->m_free && strs[m])
                            mods[m]->m_free(strs[m]);
                        strs[m] = mods[m]->m_run(mods[m]->m_data, fd);
                        dirty = true;
                    }
                }
        }
        do {
            unsigned wait = refresh_delay + (start - time);
            FD_ZERO(&fds); FD_SET(xcnf.xfd, &fds);
            ret = select(xcnf.xfd + 1, &fds, 0, 0,
                         &(struct timeval){ .tv_usec = wait });
            if (dirty || (ret > 0 && xdirty())) {
                xdrawbar(strs, packs);
                dirty = false;
            }
        } while ((time = utime()) < start + refresh_delay);
        start = time;
        puts("looping");
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
