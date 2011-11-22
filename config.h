/* config.h - xbar global configuration */

#include "mod_say.h"
#include "mod_time.h"

/* Bar font. */
static const char font[] = "fixed";

/* Bar height. */
static const int bar_height = 15;

/* Active modules and their position in the bar. */
static struct Module modules[] = {
    { .mod = MOD_SAY("xbar 2011"), .pack = PACK_RIGHT },
    { .mod = MOD_TIME(), .pack = PACK_RIGHT },
};

/* Separator used between outputs of different modules. */
static const char sep[] = " : ";

/* Default display colors. */
static const char fgcolor[] = "gray90";
static const char bgcolor[] = "black";

/* The refresh delay in nano seconds. */
static const int refresh_delay = 3000000;
