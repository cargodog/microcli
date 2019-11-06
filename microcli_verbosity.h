#ifndef _MICROCLI_VERBOSITY_H_
#define _MICROCLI_VERBOSITY_H_

#define VERBOSITY_LEVEL_DISABLED    0
#define VERBOSITY_LEVEL_ERROR       1
#define VERBOSITY_LEVEL_WARN        2
#define VERBOSITY_LEVEL_LOG         3
#define VERBOSITY_LEVEL_DEBUG       4
#define VERBOSITY_LEVEL_MAX         VERBOSITY_LEVEL_DEBUG

#ifndef DEFAULT_VERBOSITY
#define DEFAULT_VERBOSITY VERBOSITY_LEVEL_LOG
#endif

#ifndef MAX_ALLOWED_VERBOSITY
#define MAX_ALLOWED_VERBOSITY VERBOSITY_LEVEL_MAX
#endif


#define PRINT_HELPER(v, ctx, fmt, ... )\
{\
    if((ctx)->verbosity >= v) {\
        (ctx)->cfg.io.printf(fmt, ##__VA_ARGS__);\
    }\
}

#ifdef MAX_ALLOWED_VERBOSITY
    #if (MAX_ALLOWED_VERBOSITY >= VERBOSITY_LEVEL_ERROR)
        #define MICROCLI_ERROR(ctx, fmt, ...)\
            PRINT_HELPER(VERBOSITY_LEVEL_ERROR, ctx, fmt, ##__VA_ARGS__)
    #else
        #define MICROCLI_ERROR(...)
    #endif

    #if (MAX_ALLOWED_VERBOSITY >= VERBOSITY_LEVEL_WARN)
        #define MICROCLI_WARN(ctx, fmt, ...)\
            PRINT_HELPER(VERBOSITY_LEVEL_WARN, ctx, fmt, ##__VA_ARGS__)
    #else
        #define MICROCLI_WARN(...)
    #endif

    #if (MAX_ALLOWED_VERBOSITY >= VERBOSITY_LEVEL_LOG)
        #define MICROCLI_LOG(ctx, fmt, ...)\
            PRINT_HELPER(VERBOSITY_LEVEL_LOG, ctx, fmt, ##__VA_ARGS__)
    #else
        #define MICROCLI_LOG(...)
    #endif

    #if (MAX_ALLOWED_VERBOSITY >= VERBOSITY_LEVEL_DEBUG)
        #define MICROCLI_DEBUG(ctx, fmt, ...)\
            PRINT_HELPER(VERBOSITY_LEVEL_DEBUG, ctx, fmt, ##__VA_ARGS__)
    #else
        #define MICROCLI_DEBUG(...)
    #endif
#endif /* MAX_ALLOWED_VERBOSITY */

#endif /* _MICROCLI_VERBOSITY_H_ */