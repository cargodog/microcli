#ifndef _MICROCLI_CONFIG_H_
#define _MICROCLI_CONFIG_H_

#include <stdlib.h>

#define DEFAULT_VERBOSITY           VERBOSITY_LEVEL_LOG
#define MAX_ALLOWED_VERBOSITY       VERBOSITY_LEVEL_MAX

#define MAX_CLI_INPUT_LEN           100
#define MAX_CLI_CMD_LEN             20

#define MICROCLI_ENABLE_HISTORY
#define MICRICLI_MAX_HISTORY_MEM    2 * MAX_CLI_INPUT_LEN

#define MICROCLI_MALLOC             malloc
#define MICROCLI_FREE               free


#endif /* _MICROCLI_CONFIG_H_*/