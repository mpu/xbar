/* xbar 2011 */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <sys/select.h>

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
    int                 dspw;
    GC                  gc;
    XFontStruct *       fs;
    unsigned int        fg;
    unsigned int        bg;
} xcnf;

// -- Code.
static void complain(const char *);
static void term(int);

static void mloop(void);

static void xputstr(int, const char *, int);
static void xclearbar(void);
static void xdrawbar(const char **, const enum Pack *);
static bool xgetpixel(const char *, unsigned *);
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

static void
mloop(void)
{
    unsigned m, nmod = 0;
    struct ModData md[LEN(modules)];
    const struct ModInfo * mods[LEN(modules)];
    enum Pack packs[LEN(modules)];
    const char * strs[LEN(modules) + 1] = { 0 };
    bool dirty = true;

    for (m = 0; m < LEN(modules); m++) {
        if (!modules[m].mod.m_init(&md[nmod])) {
            complain("Cannot start module.");
            continue;
        }
        strs[nmod] = modules[m].mod.m_run(modules[m].mod.m_data, -1);
        mods[nmod] = &modules[m].mod;
        packs[nmod] = modules[m].pack;
        nmod++;
    }

    for (;;) {
        int ret;
        fd_set fds;

        do {
            int mxfd = 0;
            struct timeval * ts_z = &(struct timeval){ 0, 0 };

            FD_ZERO(&fds);
            for (m = 0; m < nmod; m++ ) {
                int i, fd;
                if (!(mods[m]->m_trigger & TRIG_FD))
                    continue;
                for (i = 0; (fd = md[m].md_fds[i]); i++) {
                    if (fd > mxfd)
                        mxfd = fd;
                    FD_SET(fd, &fds);
                }
            }
            ret = select(mxfd + 1, &fds, 0, 0, ts_z);
        } while (ret < 0 && (errno == EAGAIN || errno == EINTR));

        if (ret < 0) {
            complain("Select failed.");
            return;
        }

        for (m = 0; m < nmod; m++ ) {
            int i, fd;
            if (mods[m]->m_trigger & TRIG_TIME) {
                if (md[m].md_count == 0) {
                    if (strs[m] && mods[m]->m_free)
                        mods[m]->m_free(strs[m]);
                    strs[m] = mods[m]->m_run(mods[m]->m_data, -1);
                    dirty = true;
                    md[m].md_count = mods[m]->m_period;
                } else if (md[m].md_count > 0)
                    md[m].md_count--;
            }
            if (!(mods[m]->m_trigger & TRIG_FD))
                continue;
            for (i = 0; (fd = md[m].md_fds[i]); i++) {
                if (FD_ISSET(fd, &fds)) {
                    if (strs[m] && mods[m]->m_free)
                        mods[m]->m_free(strs[m]);
                    strs[m] = mods[m]->m_run(mods[m]->m_data, fd);
                    dirty = true;
                }
            }
        }
        if (dirty) {
            xdrawbar(strs, packs);
            dirty = false;
        }
        select(0, 0, 0, 0, &(struct timeval){ .tv_usec = refresh_delay });
        puts("looping");
    }
}

static void
xputstr(int x, const char * str, int len)
{
    const int y = xcnf.fs->max_bounds.ascent + 1;
    XDrawImageString(xcnf.dsp, xcnf.win, xcnf.gc, x , y, str, len);
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
    const int sep_width = XTextWidth(xcnf.fs, sep, LEN(sep) - 1);
    bool firstl, firstr;
    int xl = 0, xr = xcnf.dspw;

    xclearbar();
    for (firstl = firstr = true; *strs; packs++, strs++) {
        int len = strlen(*strs);

        switch (*packs) {
        case PACK_LEFT:
            if (!firstl) {
                xputstr(xl, sep, LEN(sep) - 1);
                xl += sep_width;
            } else
                firstl = false;
            xputstr(xl, *strs, len);
            xl += XTextWidth(xcnf.fs, *strs, len);
            break;

        case PACK_RIGHT:
            if (xr < 0)
                continue;
            if (!firstr) {
                xr -= sep_width;
                xputstr(xr, sep, LEN(sep) - 1);
            } else
                firstr = false;
            xr -= XTextWidth(xcnf.fs, *strs, len);
            xputstr(xr, *strs, len);
        }
    }
    XFlush(xcnf.dsp);
}

static bool
xgetpixel(const char * name, unsigned * pixel)
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
    xcnf.win = RootWindow(xcnf.dsp, xcnf.sn);
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
