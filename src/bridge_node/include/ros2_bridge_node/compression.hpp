// streaming single file zstd compression

#include <stdio.h>     // printf
#include <stdlib.h>    // free
#include <string.h>    // memset, strcat, strlen
#include <zstd.h>      // presumes zstd library is installed
#include "zstd_common.h"    // Helper functions, CHECK(), and CHECK_ZSTD()


static void compressFile_orDie(const char* fname, const char* outName, int cLevel, int nbThreads);
static char* createOutFilename_orDie(const char* filename);
