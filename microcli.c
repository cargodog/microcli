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

static int find_cmd_in_table(MicroCLI_t * ctx, const char * name)
{
    assert(ctx);
    assert(name);
    assert(ctx->cfg.cmdTable);
    assert(ctx->cfg.cmdCount >= 0);

    for(unsigned int i = 0; i < ctx->cfg.cmdCount; i++) {
        if(0 == strcmp(name, ctx->cfg.cmdTable[i].name))
            return i;
    }

    return MICROCLI_ERR_CMD_NOT_FOUND;
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

int microcli_parse_cmd(MicroCLI_t * ctx)
{
    #if MAX_ALLOWED_VERBOSITY > VERBOSITY_LEVEL_DISABLED
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

        // No more data to process. Wait for more data.
        if(buffer[ctx->input.len] == 0)
            return MICROCLI_ERR_NO_DATA;

        // Replace escape character with null terminator
        buffer[ctx->input.len] = 0;
        ctx->cfg.io.printf("\n\r");

        // Lookup and run command (if found)
        int cmdIdx = find_cmd_in_table(ctx, buffer);

        // Reset input buffer
        memset(&ctx->input, 0, sizeof(ctx->input));

        // Return index of command (if found)
        return cmdIdx;
    #else
        return 0;
    #endif
}

void microcli_interpreter_tick(MicroCLI_t * ctx)
{
    #if MAX_ALLOWED_VERBOSITY > VERBOSITY_LEVEL_DISABLED
        assert(ctx);
        assert(ctx->cfg.io.printf);
        assert(ctx->cfg.io.getchar);
        assert(ctx->cfg.promptText);

        // Display prompt once per command
        if(ctx->prompted == false) {
            ctx->cfg.io.printf(ctx->cfg.promptText);
            ctx->prompted = true;
        }

        // Process input data for command
        int i = microcli_parse_cmd(ctx);
        if(i >= 0 && i < ctx->cfg.cmdCount) {
            ctx->cfg.cmdTable[i].cmd();
            ctx->cfg.io.printf("\n\r");
        }
        
        // Reset prompt unless waiting for more data
        if(i != MICROCLI_ERR_NO_DATA) {
            ctx->prompted = false;
        }
    #endif
}

int microcli_banner(MicroCLI_t * ctx)
{
    #if MAX_ALLOWED_VERBOSITY > VERBOSITY_LEVEL_DISABLED
        assert(ctx);
        assert(ctx->cfg.bannerText);
        assert(ctx->cfg.io.printf);
        
        return ctx->cfg.io.printf(ctx->cfg.bannerText);
    #else
        return 0;
    #endif
}

int microcli_help(MicroCLI_t * ctx)
{
    #if MAX_ALLOWED_VERBOSITY > VERBOSITY_LEVEL_DISABLED
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
    #else
        return 0;
    #endif
}
