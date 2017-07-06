#include "ui.h"

#include <stdio.h>
#include <string.h>

static PrintConsole headerScreen, statusScreen, mainScreen, logScreen;

#define EMPTY_BAR  "                                                    "
#define CLEAR_SCREEN "\e[2J\e[H"

void uiUpdateProgress(int value, int maxValue) {
	consoleSelect(&statusScreen);
	if (maxValue <= 0) {
		printf("\x1b[1;1H    ");
		return;
	}
	int perc = value * 100 / maxValue;
	if (perc  > 100) perc = 100;
	printf("\x1b[1;1H%3d%%", perc);
	consoleSelect(&logScreen);
}

void uiUpdateStatus(char *status) {
	consoleSelect(&statusScreen);
	static int prevLength = 0;
	int len = strlen(status);
	char statusText[43];
	memset(statusText, 0, sizeof(statusText));
	if (len > 42)
		len = 42;
	strncpy(statusText, status, len);
	if (len < prevLength)
		printf("\x1b[1;6H%.*s\x1b[1;6H%s", 43, EMPTY_BAR, statusText);
	else
		printf("\x1b[1;6H%s", statusText);
	prevLength = len;
	consoleSelect(&logScreen);
}

void uiUpdateBanner() {
	consoleSelect(&headerScreen);
	printf("\e[1;7m\e[2J\e[H"); //invert colors and clear screen
	printf("\e[0;0HThenaya\e[0;32Hv0.4 [%s]", __DATE__);
	char line[51];
	memset(line, 0xc4, sizeof(line)-1);
	line[sizeof(line)-1] = '\0';
	printf("\e[1;0H\e[1;7m%s", line);
	consoleSelect(&logScreen);
}

void uiInitStatus() {
	consoleSelect(&statusScreen);
	char line[51];
	memset(line, 0xc4, sizeof(line)-1);
	line[sizeof(line)-1] = '\0';
	printf("\e[0;0H%s", line);
	consoleSelect(&logScreen);
}

// 400 x 240 = 50 cols 30 rows
// 320 x 240 = 40 cols
void uiInit() {
	gfxInitDefault();
	consoleInit(GFX_TOP, &headerScreen);
	consoleInit(GFX_TOP, &statusScreen);
	consoleInit(GFX_TOP, &mainScreen);
	consoleInit(GFX_BOTTOM, &logScreen);
	
	consoleSetWindow(&mainScreen, 0, 2, 50, 30-4);
	consoleSetWindow(&headerScreen, 0, 0, 50, 2);
	consoleSetWindow(&statusScreen, 0, 28, 50, 2);
}

void uiSelectLog() {
	consoleSelect(&logScreen);
}

void uiSelectMain() {
	consoleSelect(&mainScreen);
}

void uiExit() {
	gfxExit();
}

void uiShowMenu(MENU_TYPE type, u32 keys) {
	consoleSelect(&mainScreen);
	printf(CLEAR_SCREEN);
	switch (type) {
		case MENU_MAIN:
			if (keys & KEY_Y)
				printf("\e[1;0H X - Load tag from file.");
			if (keys & KEY_A)
				printf("\e[2;0H A - Write/Restore Tag.");
			if (keys & KEY_Y)
				printf("\e[1;26H Y - Dump Tag to file.");
			if (keys & KEY_B)
				printf("\e[2;26H B - Quit.");
		break;
		case MENU_LOAD:
		break;
		case MENU_WRITE:
		break;
		case MENU_DUMP:
		break;
	}
	consoleSelect(&logScreen);
}

void clearScreen() {
	printf("\e[2J\e[H");
}

u32 uiGetKey(u32 keys) {
	// wait till START
	while (aptMainLoop()) {
		gspWaitForVBlank();
		hidScanInput();

		u32 kDown = hidKeysDown();

		if (kDown & keys)
			return kDown;
	}
	
	return 0;
}