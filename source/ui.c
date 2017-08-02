#include "ui.h"

#include <stdio.h>
#include <string.h>

static PrintConsole headerScreen, statusScreen, mainScreen, logScreen;

#define EMPTY_BAR  "                                                    "
#define CLEAR_SCREEN "\e[2J\e[H"

void uiUpdateProgress(int value, int maxValue) {
	static int lastValue = -1;
	
	consoleSelect(&statusScreen);
	if (maxValue <= 0) {
		lastValue = -1;
		printf("\e[2;2H    ");
		consoleSelect(&logScreen);
		gfxFlushBuffers();
		return;
	}
	int perc = value * 100 / maxValue;
	if (perc  > 100) perc = 100;
	
	if (lastValue != perc) {
		value = perc;
		printf("\e[2;2H%3d%%", perc);
		gfxFlushBuffers();
	}
	consoleSelect(&logScreen);
}

void uiUpdateStatus(char *status) {
	consoleSelect(&statusScreen);
	printf("\e[2;7H%-40.40s", status); //print left justified string max min 40 chars
	gfxFlushBuffers();
	consoleSelect(&logScreen);
}

void uiUpdateBanner() {
	consoleSelect(&headerScreen);
	printf("\e[1;7m\e[2J\e[H"); //invert colors and clear screen
	printf("\e[1;1HThenaya\e[1;33Hv%d.%d [%s]", MAJOR_VERSION, MINOR_VERSION, __DATE__);
	char line[51];
	memset(line, 0xc4, sizeof(line)-1);
	line[sizeof(line)-1] = '\0';
	printf("\e[2;1H\e[1;7m%s", line);
	gfxFlushBuffers();
	consoleSelect(&logScreen);
}

void uiInitStatus() {
	consoleSelect(&statusScreen);
	char line[51];
	memset(line, 0xc4, sizeof(line)-1);
	line[sizeof(line)-1] = '\0';
	printf("\e[1;1H%s", line);
	gfxFlushBuffers();
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

void uiClearScreen() {
	printf("\e[2J\e[H");
}

u32 uiGetKey(u32 keys) {
	// wait till START
	while (aptMainLoop()) {
		gfxFlushBuffers();
		gfxSwapBuffers();
		gspWaitForVBlank();

		hidScanInput();

		u32 kDown = hidKeysDown();

		if (kDown & keys)
			return kDown;
	}
	
	return 0;
}