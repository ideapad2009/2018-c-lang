#include <stdio.h>
#include <unistd.h>
#include "userconfig.h"

int FatfsComboWrite(const void* buffer, int size, int count, FILE* stream)
{
    int res = 0;
    res = fwrite(buffer, size, count, stream);
    res |= fflush(stream);        // required by stdio, this will empty any buffers which newlib holds
#if IDF_3_0
    res |= fsync(fileno(stream)); // this will tell the filesystem driver to write data to disk
#endif

    return res;
}

