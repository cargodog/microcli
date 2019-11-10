#include "microcli.h"
#include <stdio.h>


#define CMD_ENTRY(fn, help) {fn, #fn, help}

int poke(MicroCLI_t * ctx, const char * args);
int help(MicroCLI_t * ctx, const char * args);

MicroCLI_t dbg;
const MicroCLICmdEntry_t cmdTable[] = {
    CMD_ENTRY(help, "Print this help message"),
    CMD_ENTRY(poke, "Poke the poor circuit"),
};
const MicroCLICfg_t dbgCfg = {
    .bannerText = "\r\n\n\n\nMicroCLI Interpreter Demo\r\n",
    .promptText = "> ",
    .cmdTable = cmdTable,
    .cmdCount = sizeof(cmdTable)/sizeof(cmdTable[0]),
    .io.printf = printf,
    .io.getchar = getchar,
};


int poke(MicroCLI_t * ctx, const char * args)
{
    microcli_log(ctx, "ouch!\n\r");
    return 0;
}

int help(MicroCLI_t * ctx, const char * args)
{
    return microcli_help(ctx);
}

int main(void)
{
    microcli_init(&dbg, &dbgCfg);
    microcli_banner(&dbg);
    while(1)
        microcli_interpreter_tick(&dbg);
}