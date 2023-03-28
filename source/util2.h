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

void printbuf(char *prefix, uint8_t* data, size_t len);
int readFile(char *filepath, uint8_t *data, u32 datasize);
int writeFile(char *filepath, uint8_t *data, u32 datasize);
void cleanFilename(char *filename);

#ifdef __cplusplus
}
#endif
