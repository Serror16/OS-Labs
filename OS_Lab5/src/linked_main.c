#include "commands.h"

int main(void)
{
    LibraryFunctions functions = {GCF, Sort};
    return run_commands(&functions, 0, 0);
}
