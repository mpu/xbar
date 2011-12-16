#include <string.h>

#include <unistd.h>

#include "xbar.h"
#include "mod_read.h"

#define LINE_LEN 512
#define BUF_LEN 512

struct LineData {
    size_t      sz;
    char        buf[BUF_LEN];
};

static bool mod_read_input_line(struct LineData *, char *, int);

static bool
mod_read_input_line(struct LineData * ld, char * line, int len)
{
    size_t bol, cr;
    ssize_t ret;

    ret = read(STDIN_FILENO, &ld->buf[ld->sz], BUF_LEN - ld->sz);
    if (ret <= 0 && ld->sz == 0)
        return ret == 0;
    ld->sz += ret;
    cr = ld->sz - 1;
    while (cr > 0 && ld->buf[cr] != '\n')
        cr--;
    if (cr) { // Find beginning of line.
        bol = cr;
        while (bol > 0 && ld->buf[bol - 1] != '\n')
            bol--;
    } else { // Otherwise, take the whole line.
        cr = ld->sz - 1;
        bol = cr && ld->buf[0] == '\n' ? 1 : 0;
    }
    ld->buf[cr] = 0;
    strncpy(line, &ld->buf[bol], len);
    line[len - 1] = 0;
    memmove(ld->buf, &ld->buf[cr + 1], BUF_LEN - cr - 1);
    ld->sz -= cr + 1;
    return false;
}

bool
mod_read_init(struct ModData * pmd)
{
    static int fds[] = { STDIN_FILENO, -1 };
    *pmd = (struct ModData) {
        .count = -1,
        .fds = fds,
    };
    return true;
}

enum ModStatus
mod_read_run(const char ** ret, void * p, int fd)
{
    static struct LineData ld;
    static char line[LINE_LEN] = "Waiting for input...";
    bool eof = false;

    (void) p;
    if (fd >= 0) {
        assert(fd == STDIN_FILENO);
        eof = mod_read_input_line(&ld, line, LINE_LEN);
    }
    *ret = line;
    return eof ? ST_EOF : ST_OK;
}
