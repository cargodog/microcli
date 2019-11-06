#include "microcli.h"
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>


// Special characters:
enum {
    ESC = 27,
    SEQ = 91,
    UP_ARROW = 65,
    DOWN_ARROW = 66,
};

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

static inline void save_input_to_history(MicroCLI_t * ctx)
{
    #ifdef MICROCLI_ENABLE_HISTORY
        assert(ctx);
        assert(ctx->historySize <= MICRICLI_MAX_HISTORY_MEM); // Memory overflow?
        assert((ctx->history != 0) != (ctx->historySize == 0)); // Memory leak?
        assert(ctx->input.len <= MAX_CLI_INPUT_LEN); // Input overflow?
        assert(ctx->input.buffer[ctx->input.len-1] == 0); // Null terminated?

        static const unsigned int NODE_SIZE = sizeof(struct MicroCLIHistEntry);

        // Abort if this entry is too large to possibly be stored
        if(ctx->input.len > MICRICLI_MAX_HISTORY_MEM - NODE_SIZE)
            return;
        
        // Abort if there is no data to save
        if(ctx->input.len <= 1)
            return;
        
        // Seek last history entry
        struct MicroCLIHistEntry * last = ctx->history;
        while(last && last->older) {
            assert(last != last->older); // Circular reference?
            last = last->older;
        }

        // Calculate new history size
        ctx->historySize += NODE_SIZE + ctx->input.len;

        // Free older entries, to prevent exceeding history max mem
        while(last && ctx->historySize > MICRICLI_MAX_HISTORY_MEM) {
            struct MicroCLIHistEntry * this = last;

            // Trim the last node from the list
            last = last->newer;
            if(last)
                last->older = 0;

            // Free the allocated string memory within this node
            if(this && this->str) {
                MICROCLI_FREE(this->str);
                ctx->historySize -= strnlen(this->str, MAX_CLI_INPUT_LEN) + 1;
            }

            // Free the memory of this node itself
            MICROCLI_FREE(this);
            ctx->historySize -= NODE_SIZE;

            // Is the list empty?
            if(ctx->history == this)
                ctx->history = 0;
        }

        // Allocate memory for new list node
        struct MicroCLIHistEntry * newEntry = MICROCLI_MALLOC(NODE_SIZE);
        assert(newEntry); // Malloc failure?

        // Insert new list node at front of list
        newEntry->newer = 0; // There is no newer entry
        newEntry->older = ctx->history;
        ctx->history = newEntry;
        if(newEntry->older)
            newEntry->older->newer = newEntry;

        // Allocate memory for the input string
        newEntry->str = MICROCLI_MALLOC(ctx->input.len);
        assert(newEntry->str); // Malloc failure?

        // Save the input string
        strcpy(newEntry->str, ctx->input.buffer);
    #endif
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

static inline void clear_line(MicroCLI_t * ctx)
{
    assert(ctx);
    assert(ctx->cfg.io.printf);
    
    ctx->cfg.io.printf("\r%c%c%c", ESC, SEQ, 'K');
}

static inline void clear_prompt(MicroCLI_t * ctx)
{
    assert(ctx);
    assert(ctx->cfg.io.printf);
    assert(ctx->cfg.promptText);
    
    clear_line(ctx);
    ctx->cfg.io.printf("\r%s", ctx->cfg.promptText);
}

static inline void print_history_entry( MicroCLI_t * ctx,
                                        struct MicroCLIHistEntry * entry)
{
    assert(ctx);
    assert(ctx->cfg.io.printf);
    assert(entry);
    assert(entry->str);

    // Clear the line
    clear_prompt(ctx);

    // Print the history entry text
    ctx->cfg.io.printf("%s", entry->str);
}

// Fill input buffer from IO stream. Return true when buffer is ready to process.
static void read_from_io(MicroCLI_t * ctx)
{
    assert(ctx);
    assert(ctx->cfg.io.printf);
    assert(ctx->cfg.io.getchar);

    // Convenience mapping
    char * buffer = ctx->input.buffer;

    // Current history entry
    static struct MicroCLIHistEntry * hist = 0;

    // Loop through all available data
    while(ctx->input.len < (sizeof(ctx->input.buffer) - 1)) {
        // Get next character
        buffer[ctx->input.len] = ctx->cfg.io.getchar();

        // Handle termination characters
        if( buffer[ctx->input.len] == 0 ||     // No more data
            buffer[ctx->input.len] == '\n' ||  // New line
            buffer[ctx->input.len] == '\r' )   // Carriage return
            break;

        // Handle escape sequence
        if( buffer[ctx->input.len - 2] == ESC &&
            buffer[ctx->input.len - 1] == SEQ ) {

            // Handle the special character
            bool printFromHistory = false;
            switch(buffer[ctx->input.len]) {
                case UP_ARROW:
                    if(hist && hist->older) {
                        hist = hist->older;
                        printFromHistory = true;
                    } else if(!hist && ctx->history) {
                        hist = ctx->history;
                        printFromHistory = true;
                    }
                    break;
                case DOWN_ARROW:
                    if(hist) {
                        hist = hist->newer;
                        if(hist) {
                            printFromHistory = true;
                        } else {
                            // Reset back to input buffer
                            clear_prompt(ctx);
                            ctx->cfg.io.printf(ctx->input.buffer);
                        }
                    }
                    break;
            }
            if(printFromHistory && hist->str)
                print_history_entry(ctx, hist);

            // Clear the sequence from the input buffer
            buffer[ctx->input.len--] = 0;
            buffer[ctx->input.len--] = 0;
            buffer[ctx->input.len--] = 0;
        }

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
            // Echo back if not an escape sequence
            if( buffer[ctx->input.len] != ESC &&
                buffer[ctx->input.len - 1] != ESC )
                ctx->cfg.io.printf("%c", buffer[ctx->input.len]);
            
            ctx->input.len++;
        }
    }
    assert(ctx->input.len < MAX_CLI_INPUT_LEN);

    // If buffer is full, delete last char to make room for more input
    if(ctx->input.len == sizeof(ctx->input.buffer)) {
        ctx->cfg.io.printf("\b");
        ctx->input.len--;
        buffer[ctx->input.len] = 0;
    }
    assert(ctx->input.len < MAX_CLI_INPUT_LEN);

    // No more data to process. Wait for more data.
    if(buffer[ctx->input.len] == 0) {
        ctx->input.ready = false;
    } else {
        // Was a historical entry selected?
        if(hist && hist->str) {
            strcpy(ctx->input.buffer, hist->str);
            ctx->input.len = strnlen(hist->str, MAX_CLI_INPUT_LEN);
        }
        hist = 0;

        // Replace escape character with null terminator
        buffer[ctx->input.len++] = 0;
        ctx->cfg.io.printf("\n\r");

        // Command entry is complete. Input buffer is ready to be processed
        ctx->input.ready = true;
        assert(ctx->input.len < MAX_CLI_INPUT_LEN);
        save_input_to_history(ctx);
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
