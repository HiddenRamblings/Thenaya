#pragma once

#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>


#ifdef __cplusplus
extern "C" {
#endif

void printbuf(char *prefix, u8* data, size_t len);
int readFile(char *filepath, u8 *data, u32 datasize);
int writeFile(char *filepath, u8 *data, u32 datasize);
void cleanFilename(char *filename);

#ifdef __cplusplus
}
#endif
