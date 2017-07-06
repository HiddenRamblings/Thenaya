#pragma once

#include <3ds.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { MENU_MAIN, MENU_LOAD, MENU_WRITE, MENU_DUMP } MENU_TYPE;


void uiSelectLog();
void uiSelectMain();
void uiUpdateProgress(int value, int maxValue);
void uiUpdateStatus(char *status);
void uiUpdateBanner();
void uiInitStatus();
u32 uiGetKey(u32 keys);
void uiClearScreen();
void uiInit();
void uiExit();

#ifdef __cplusplus
}
#endif
