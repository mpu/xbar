#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "xbar.h"
#include "mod_cmd.h"

#define LINE_LEN 512
#define SHELL "/bin/sh"

static bool mod_cmd_fork_process(pid_t *, int *, const char *);

static bool
mod_cmd_fork_process(pid_t * ppid, int * pfd, const char * cmd)
{
    pid_t pid;
    int fildes[2];

    if (pipe(fildes) < 0) {
        perror("mod_cmd_fork_process");
        return false;
    }
    if ((pid = fork()) == 0) {
        const char * const args[] = { SHELL, "-c", cmd, 0 };
        close(fildes[0]);
        dup2(fildes[1], STDOUT_FILENO);
        execv(SHELL, (char * const *)args); // crappy type here.
        puts("mod_cmd: execv failed");
        abort();
    } else if (pid > 0) {
        close(fildes[1]);
        *pfd = fildes[0];
        *ppid = pid;
        return true;
    } else {
        perror("mod_cmd_fork_process");
        return false;
    }
}

bool
mod_cmd_init(struct ModData * pmd)
{
    struct ModData md = {
        .md_count = 0,
        .md_fds = 0,
    };
    *pmd = md;
    return true;
}

const char *
mod_cmd_run(void * p, int ufd)
{
    static const char err[] = "mod_cmd: Error";
    static char line[LINE_LEN];
    char * cr;
    ssize_t rd;
    pid_t pid;
    int status, fd;

    (void) ufd;
    if (!mod_cmd_fork_process(&pid, &fd, (const char *)p))
        return err;
    rd = read(fd, line, sizeof line - 1);
    close(fd);
    waitpid(pid, &status, 0);
    if (rd < 0) {
        perror("mod_cmd_run");
        return err;
    }
    line[rd] = 0;
    if ((cr = strchr(line, '\n')))
        *cr = 0;
    return line;
}
