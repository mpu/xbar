/* config.h - xbar global configuration */

#include "mod_say.h"
#include "mod_time.h"
#include "mod_cpu.h"
#include "mod_bat.h"

/* Bar font. */
static const char font[] = "fixed";

/* Bar height. */
static const int bar_height = 15;

/* Active modules and their position in the bar. */
static struct Module modules[] = {
    { .mod = MOD_SAY("xbar 2011"), .pack = PACK_RIGHT },
    { .mod = MOD_TIME(), .pack = PACK_RIGHT },
    { .mod = MOD_CPU("cpu: %u%%"), .pack = PACK_LEFT },
    { .mod = MOD_BAT("BAT0", "bat: %u%%"), .pack = PACK_LEFT },
};

/* Separator used between outputs of different modules. */
static const char sep[] = " // ";

/* Default display colors. */
static const char fgcolor[] = "gray90";
static const char bgcolor[] = "black";

/* The refresh delay in nano seconds. */
static const int refresh_delay = 1500000;
