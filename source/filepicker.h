#pragma once

#define PICK_FILE_SIZE 1024

#ifdef __cplusplus
extern "C" {
#endif

int fpPickFile(const char *path, char *selectedFile, unsigned int filenameSize);

#ifdef __cplusplus
}
#endif
