// To be run with LD_LIBRARY_PATH=. ./hookmain
// For libTAS, both runpath and libpath must be set to the program directory

#include "hooktest.h"

#include <stdio.h>

int main()
{
    printf("Running hooktest from main\n");
    hooktest();
}
