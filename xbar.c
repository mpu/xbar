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

// -- Code.
static void complain(const char *);
static void term(int);
static long long utime(void);

static void mloop(void);

static bool xdirty(void);
static void xputstr(int *, enum Pack, const char *, int);
static void xclearbar(void);
static void xdrawbar(const char **, const enum Pack *);
static bool xgetpixel(const char *, unsigned long *);
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
    return (tv.tv_sec * 1000000 + tv.tv_usec);
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
xputstr(int * x, enum Pack pack, const char * str, int len)
{
    const int y = xcnf.fs->max_bounds.ascent + 1;
    const int wid = XTextWidth(xcnf.fs, str, len);

    switch (pack) {
    case PACK_LEFT:
        XDrawImageString(xcnf.dsp, xcnf.win, xcnf.gc, *x , y, str, len);
        *x += wid;
        break;

    case PACK_RIGHT:
        *x -= wid;
        XDrawImageString(xcnf.dsp, xcnf.win, xcnf.gc, *x , y, str, len);
        break;
    }
}

static void
xclearbar(void)
{
    const int clear_width = xcnf.dspw < 0 ? 800 : xcnf.dspw;

    XSetForeground(xcnf.dsp, xcnf.gc, xcnf.bg);
    XFillRectangle(xcnf.dsp, xcnf.win, xcnf.gc,
                   0, 0, clear_width, bar_height);
    XSetForeground(xcnf.dsp, xcnf.gc, xcnf.fg);
}

static void
xdrawbar(const char ** strs, const enum Pack * packs)
{
    bool first[2] = { true, true };
    int x[2] = { [PACK_RIGHT] = xcnf.dspw < 0 ? 800 : xcnf.dspw };

    xclearbar();
    for (; *strs; packs++, strs++) {
        assert(*packs < 2);
        if (!first[*packs])
            xputstr(&x[*packs], *packs, sep, LEN(sep) - 1);
        else
            first[*packs] = false;
        xputstr(&x[*packs], *packs, *strs, strlen(*strs));
    }
    XFlush(xcnf.dsp);
}

static bool
xgetpixel(const char * name, unsigned long * pixel)
{
    XColor xc;

    if (!XAllocNamedColor(xcnf.dsp, DefaultColormap(xcnf.dsp, xcnf.sn),
                          name, &xc, &xc)) {
        complain("Cannot allocate color.");
        return false;
    }
    *pixel = xc.pixel;
    return true;
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
        xcnf.dspw = -1;
    } else
        xcnf.dspw = wattr.width;
    if ((xcnf.fs = XLoadQueryFont(xcnf.dsp, font)) == NULL) {
        XCloseDisplay(xcnf.dsp);
        complain("Cannot load font.");
        return false;
    }
    if (!xgetpixel(fgcolor, &xcnf.fg) || !xgetpixel(bgcolor, &xcnf.bg)) {
        xcnf.fg = WhitePixel(xcnf.dsp, xcnf.sn);
        xcnf.bg = BlackPixel(xcnf.dsp, xcnf.sn);
    }
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
