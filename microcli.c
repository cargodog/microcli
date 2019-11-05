#include "microcli.h"
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>


static void insert_spaces(MicroCLI_t * ctx, int num)
{
    assert(ctx);
    assert(num > 0);
    
    for(int i = 0; i < num; i++) {
        ctx->cfg.io.printf(" ");
    }
}

static inline int cmd_len(const char * cmdStr)
{
    assert(cmdStr);

    int inLen = strlen(cmdStr);
    int i = 0;
    for(i = 0; i < inLen; i++) {
        if(cmdStr[i] == ' ')
            break;
    }

    return i;
}

static int lookup_command(MicroCLI_t * ctx)
{
    assert(ctx);
    assert(ctx->cfg.cmdTable);
    assert(ctx->cfg.cmdCount >= 0);

    // Loop through each possible command
    int i = ctx->cfg.cmdCount;
    while(i-- > 0) {
        // Compare the input command (delimited by a space) to
        // the command in the table
        bool diff = false;
        int j = strlen(ctx->cfg.cmdTable[i].name);
        if(j > cmd_len(ctx->input.buffer))
            continue;
        
        while(j-- > 0 && !diff) {
            if(ctx->input.buffer[j] != ctx->cfg.cmdTable[i].name[j])
                diff = true;
        }

        if(!diff)
            return i;
    }

    return MICROCLI_ERR_CMD_NOT_FOUND;
}

static inline void prompt_for_input(MicroCLI_t * ctx)
{
    assert(ctx);
    assert(ctx->cfg.io.printf);

    // Only display prompt once per command
    if(ctx->prompted == false) {
        ctx->cfg.io.printf(ctx->cfg.promptText);
        ctx->prompted = true;
    }
}

// Fill input buffer from IO stream. Return true when buffer is ready to process.
static void read_from_io(MicroCLI_t * ctx)
{
    assert(ctx);
    assert(ctx->cfg.io.printf);
    assert(ctx->cfg.io.getchar);

    // Convenience mapping
    char * buffer = ctx->input.buffer;

    // Loop through all available data
    while(ctx->input.len < sizeof(ctx->input.buffer)) {
        // Get next character
        buffer[ctx->input.len] = ctx->cfg.io.getchar();

        // Handle escape characters
        if( buffer[ctx->input.len] == 0 ||     // No more data
            buffer[ctx->input.len] == '\n' ||  // New line
            buffer[ctx->input.len] == '\r' )   // Carriage return
            break;

        // Handle backspace
        if(buffer[ctx->input.len] == '\b') {
            if(ctx->input.len > 0) {
                // Overwrite prev char from screen
                ctx->cfg.io.printf("\b \b");
                ctx->input.len--;
            }

            // Erase from buffer
            buffer[ctx->input.len] = 0;
        } else {
            // Echo back
            ctx->cfg.io.printf("%c", buffer[ctx->input.len]);
            ctx->input.len++;
        }
    }

    // If buffer is full, delete last char to make room for more input
    if(ctx->input.len == sizeof(ctx->input.buffer)) {
        ctx->cfg.io.printf("\b");
        ctx->input.len--;
        buffer[ctx->input.len] = 0;
    }

    // No more data to process. Wait for more data.
    if(buffer[ctx->input.len] == 0) {
        ctx->input.ready = false;
    } else {
        // Replace escape character with null terminator
        buffer[ctx->input.len] = 0;
        ctx->cfg.io.printf("\n\r");

        // Command entry is complete. Input buffer is ready to be processed
        ctx->input.ready = true;
    }
}

void microcli_init(MicroCLI_t * ctx, const MicroCLICfg_t * cfg)
{
    assert(ctx);
    assert(cfg);
    assert(cfg->io.printf);
    assert(cfg->io.getchar);
    assert(cfg->bannerText);
    assert(cfg->promptText);
    assert(cfg->cmdTable);
    assert(cfg->cmdCount >= 0);

    *ctx = (MicroCLI_t){0};

    ctx->cfg = *cfg;
    ctx->verbosity = DEFAULT_VERBOSITY;
}

void microcli_set_verbosity(MicroCLI_t * ctx, int verbosity)
{
    assert(ctx);
    assert(verbosity <= VERBOSITY_LEVEL_MAX);
    ctx->verbosity = verbosity;
}

void microcli_interpreter_tick(MicroCLI_t * ctx)
{
    assert(ctx);
    assert(ctx->cfg.io.printf);
    assert(ctx->cfg.io.getchar);
    assert(ctx->cfg.promptText);

    // If input buffer does not contain a command, read more data from IO
    if(!ctx->input.ready) {
        // Prompt user for input
        prompt_for_input(ctx);

        // Read user input from IO
        read_from_io(ctx);
    }
    
    // If the input buffer contains a command, process it.
    if(ctx->input.ready) {
        // Lookup and run command (if found)
        int cmdIdx = lookup_command(ctx);
        const char * args = ctx->input.buffer + cmd_len(ctx->input.buffer) + 1;
        if(cmdIdx >= 0 && cmdIdx < ctx->cfg.cmdCount) {
            ctx->cfg.cmdTable[cmdIdx].cmd(args);
            ctx->cfg.io.printf("\n\r");
        }

        // Reset prompt
        ctx->prompted = false;
    }

    // Reset the prompt, after processing a command
    if(ctx->input.ready) {
        memset(&ctx->input, 0, sizeof(ctx->input));
        ctx->prompted = false;
    }
}

int microcli_interpret_string(MicroCLI_t * ctx, const char * str, bool print)
{
    assert(ctx);
    assert(ctx->cfg.io.printf);
    assert(str);

    if(strlen(str) > sizeof(ctx->input.buffer))
        return MICROCLI_ERR_OVERFLOW;

    if( (ctx->input.len > 0) || ctx->input.ready )
        return MICROCLI_ERR_BUSY;
    
    // Copy string into the command input buffer
    strcpy(ctx->input.buffer, str);
    ctx->input.len = strlen(str);
    ctx->input.ready = true;
    
    // Optionally print the command to the cli output
    if(print) {
        // Display the prompt, before printing the command
        prompt_for_input(ctx);
        
        // Print the command to the console
        ctx->cfg.io.printf("%s\n\r", str);
    }

    // Tick the interpreter to process the command
    microcli_interpreter_tick(ctx);

    return 0;
}

int microcli_banner(MicroCLI_t * ctx)
{
    assert(ctx);
    assert(ctx->cfg.bannerText);
    assert(ctx->cfg.io.printf);
    
    return ctx->cfg.io.printf(ctx->cfg.bannerText);
}

int microcli_help(MicroCLI_t * ctx)
{
    assert(ctx);
    assert(ctx->cfg.io.printf);
    assert(ctx->cfg.cmdTable);
    assert(ctx->cfg.cmdCount >= 0);

    int printLen = 0;
    for(int i = 0; i < ctx->cfg.cmdCount; i++) {
        int cmdLen = ctx->cfg.io.printf("%s", ctx->cfg.cmdTable[i].name);
        insert_spaces(ctx, MAX_CLI_CMD_LEN - cmdLen);
        printLen += MAX_CLI_CMD_LEN;
        printLen += ctx->cfg.io.printf("%s\n\r", ctx->cfg.cmdTable[i].help);
    }
    
    return printLen;
}
