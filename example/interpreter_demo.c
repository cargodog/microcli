#include "microcli.h"
#include <stdio.h>


int poke(void);
int help(void);

MicroCLI_t dbg;
const MicroCLICmdEntry_t cmdTable[] = {
    {"poke", poke, "Poke the poor circuit"},
    {"help", help, "Print this help message"},
};
const MicroCLICfg_t dbgCfg = {
    .bannerText = "\r\n\n\n\nMicroCLI Interpreter Demo\r\n",
    .promptText = "> ",
    .cmdTable = cmdTable,
    .cmdCount = sizeof(cmdTable)/sizeof(cmdTable[0]),
    .io.printf = printf,
    .io.getchar = getchar,
};


int poke(void)
{
    microcli_log(&dbg, "ouch!\n\r");
    return 0;
}

int help(void)
{
    return microcli_help(&dbg);
}

int main(void)
{
    microcli_init(&dbg, &dbgCfg);
    microcli_banner(&dbg);
    while(1)
        microcli_interpreter_tick(&dbg);
}