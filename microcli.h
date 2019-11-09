#ifndef _MICROCLI_H_
#define _MICROCLI_H_

#include "microcli_config.h"
#include "microcli_verbosity.h"
#include <stdbool.h>


// Error codes
typedef enum {
    MICROCLI_ERR_UNKNOWN = -128,
    MICROCLI_ERR_BUSY,
    MICROCLI_ERR_BAD_ARG,
    MICROCLI_ERR_OVERFLOW,
    MICROCLI_ERR_OUT_OF_BOUNDS,
    MICROCLI_ERR_CMD_NOT_FOUND,
    MICROCLI_ERR_NO_DATA,
    MICROCLI_ERR_MAX
} MicroCLIErr_t;
_Static_assert(MICROCLI_ERR_MAX < 0, "Some MicroCLI error codes are non-negative!");


// Typedefs
typedef struct microcliHistoryEntry MicroCLIHistoryEntry_t;
typedef struct microcliCmdEntry     MicroCLICmdEntry_t;
typedef struct microcliCfg          MicroCLICfg_t;
typedef struct microcliCtx          MicroCLI_t;

// Command function prototype
typedef int (*MicroCLICmd_t)(MicroCLI_t * ctx, const char * args);

// I/O function prototypes
typedef int (*Printf_t)(const char * fmt, ...);
typedef int (*Getchar_t)(void);

struct microcliCmdEntry {
    MicroCLICmd_t cmd;
    const char * name;
    const char * help;
};

struct microcliHistoryEntry {
    char * str;
    MicroCLIHistoryEntry_t * newer;
    MicroCLIHistoryEntry_t * older;
};

struct microcliCfg {
    struct {
        Printf_t printf;
        Getchar_t getchar; // Note: should return < 0 when no more data available
    } io;
    const char * bannerText;
    const char * promptText;
    const MicroCLICmdEntry_t * cmdTable;
    unsigned int cmdCount;
};

struct microcliCtx {
    MicroCLICfg_t cfg;
    int verbosity;
    bool prompted;
    struct {
        char buffer[MAX_CLI_INPUT_LEN + 1];
        unsigned int len;
        bool ready;
    } input;
    MicroCLIHistoryEntry_t * history;
    unsigned int historySize;
};


// Control
void             microcli_init(MicroCLI_t * ctx, const MicroCLICfg_t * cfg);
void    microcli_set_verbosity(MicroCLI_t * ctx, int verbosity);
void microcli_interpreter_tick(MicroCLI_t * ctx);

// Input
int microcli_interpret_string(MicroCLI_t * ctx, const char * str, bool print);

// Output
int    microcli_banner(MicroCLI_t * ctx);
int      microcli_help(MicroCLI_t * ctx);
#define microcli_error(ctx, fmt, ...) MICROCLI_ERROR(ctx, fmt, ##__VA_ARGS__)
#define  microcli_warn(ctx, fmt, ...) MICROCLI_WARN(ctx, fmt, ##__VA_ARGS__)
#define   microcli_log(ctx, fmt, ...) MICROCLI_LOG(ctx, fmt, ##__VA_ARGS__)
#define microcli_debug(ctx, fmt, ...) MICROCLI_DEBUG(ctx, fmt, ##__VA_ARGS__)

#endif /* _MICROCLI_H_ */