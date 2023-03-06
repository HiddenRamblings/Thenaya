#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include <3ds.h>
#include <3ds/errf.h>
#include <3ds/services/nfc.h>
#include <3ds/services/apt.h>

#include "tag.h"
#include "nfc.h"
#include "elite.h"
#include "nfc3d/amitool.h"
#include "filepicker.h"
#include "ui.h"
#include "amiibolookup.h"
#include "util2.h"

#define KEY_FILE_PATH "sdmc:/key_retail.bin"
#define KEY_FILE_SIZE 160

#define AMIIBO_DUMP_ROOT "sdmc:/amiibo"

void printbuf(char *prefix, u8* data, size_t len);
void uiShowTagInfo();

int loadKeys() {
	uiSelectLog();
	u8 keybuffer[KEY_FILE_SIZE];
	printf("Reading key file %s\n", KEY_FILE_PATH);
	int size = readFile(KEY_FILE_PATH, keybuffer, KEY_FILE_SIZE);
	if (size < 0) {
		printf("Failed to read key file: %d\n", size);
		uiUpdateStatus("ERROR");
		return 0;
	}
	if (size != KEY_FILE_SIZE) {
		printf("Failed to read key file (file size incorrect: %d)\n", size);
		uiUpdateStatus("ERROR");
		return 0;
	}

	if (tag_setKeys(keybuffer, size) != 0) {
		printf("Invalid key file");
		uiUpdateStatus("ERROR");
		return 0;
	}

	printf("Key file loaded.\n");
	return 1;
}

void loadDump() {
	uiUpdateStatus("Select file");
	char filename[PICK_FILE_SIZE];
	if (!fpPickFile(AMIIBO_DUMP_ROOT, filename, sizeof(filename))) {
		printf("No file selected\n");
		goto END_loadDump;
	}
	uiSelectLog();
	printf("File selected %s\n", filename);
	u8 tagdata[AMIIBO_MAX_SIZE];
	int size = readFile(filename, tagdata, AMIIBO_MAX_SIZE);
	if (size < 0) {
		printf("Failed to read key file: %d\n", size);
		goto END_loadDump;
	}

	if (!tag_isValid(tagdata, size)) {
		printf("Not a valid tag file\n");
		goto END_loadDump;
	}

	int res = tag_setTag(tagdata, size);
	if (res != TAG_ERR_OK) {
		printf("Failed to load tag: %d\n", res);
		goto END_loadDump;
	}
	return;

END_loadDump:
	uiUpdateStatus("ERROR");
	uiSelectMain();
	printf("\e[2J\e[H\e[0m\e[5;2HLoad tag file failed.\n\n   Press A to continue.");
	uiGetKey(KEY_A);
	uiUpdateStatus("");
}

void writeToTag() {
	if (!tag_isKeysLoaded()) {
		printf("No keys loaded\n");
		return;
	}
	if (!tag_isLoaded()) {
		printf("No tag loaded\n");
		return;
	}

	uiSelectMain();
	//todo: show title as write to tag / restore tag
	printf("\e[2J\e[H\e[0m\e[5;2HPlace tag on scanner, or press B to cancel");
	uiUpdateStatus("Waiting...");
	uiSelectLog();
	u8 firstPages[NTAG_BLOCK_SIZE];

	int res = nfc_readBlock(0, firstPages, sizeof(firstPages));
	if (res != 0) {
		printf("Failed to get UID: %d\n", res);
		goto writeToTag_ERROR;
	}

	printf("Got new UID\n");
	res = tag_setUid(firstPages, 9);
	if (res != TAG_ERR_OK) {
		printf("Failed to update UID: %d\n", res);
		goto writeToTag_ERROR;
	}
	uiUpdateStatus("Encrypting.");
	printf("Encrypting...\n");
	u8 data[AMIIBO_MAX_SIZE];
	res = tag_getTag(data, sizeof(data));
	if (res != TAG_ERR_OK) {
		printf("Failed to encrypt tag: %d\n", res);
		goto writeToTag_ERROR;
	}
	/*
	printf("Backup...\n");
	res = writeFile("sdmc:/amiibo/out.bin", data, sizeof(data));
	if (res <0)
		printf("Write to disk failed: %d\n", res);
	*/

	u8 uid[7];
	res = tag_getUidFromBlock(firstPages, sizeof(firstPages), uid, sizeof(uid));
	if (res != TAG_ERR_OK) {
		printf("Failed to get uid: %d\n", res);
		goto writeToTag_ERROR;
	}

	printf("Calculating password...\n");
	u8 pwd[NTAG_PAGE_SIZE];
	res = tag_calculatePassword(uid, sizeof(uid), pwd, sizeof(pwd));
	if (res != TAG_ERR_OK) {
		printf("Failed to calculate pwd: %d\n", res);
		goto writeToTag_ERROR;
	}

	if (tag_isLocked(firstPages, sizeof(firstPages))) {
		//already an amiibo, write only game data
		printf("Locked tag. Writing game data..\n");
		res = nfc_write(data, sizeof(data), pwd, sizeof(pwd), 0);
		if (res != 0) {
			printf("nfc write failed %d\n", res);
			goto writeToTag_ERROR;
		}
	} else {
		//blank tag. write full amiibo
		printf("Writing to blank tag...\n");
		res = nfc_write(data, sizeof(data), pwd, sizeof(pwd), 1);
		if (res != 0) {
			printf("nfc write failed %d\n", res);
			goto writeToTag_ERROR;
		}
	}

	uiUpdateStatus("Complete.");
	uiUpdateProgress(0, -1);
	uiSelectMain();
	printf("\e[2J\e[H\e[0m\e[5;2HFinished writing to tag.\n\n   Press A to continue.");
	uiGetKey(KEY_A);
	return;

writeToTag_ERROR:
	uiUpdateStatus("ERROR");
	uiSelectMain();
	printf("\e[2J\e[H\e[0m\e[5;2HWrite to tag failed.\n\n   Press A to continue.");
	uiGetKey(KEY_A);
	uiUpdateStatus("");
}

void dumpTagToFile() {
	uiSelectMain();
	//todo: show title as write to tag / restore tag
	printf("\e[2J\e[H\e[0m\e[5;2HPlace tag on scanner, or press B to cancel");
	uiUpdateStatus("Waiting...");
	u8 data[AMIIBO_MAX_SIZE];
	uiSelectLog();
	int res = nfc_readFull(data, sizeof(data));
	if (res != 0) {
		printf("Scanning failed\n");
		goto dumpTagToFile_ERROR;
	}

	if (!tag_isValid(data, sizeof(data))) {
		printf("WARNING: Likely not an amiibo.\n");
	}

	uiUpdateStatus("Saving..");
	mkdir(AMIIBO_DUMP_ROOT, 0777);

	char tagName[MAX_AMIIBO_NAME];
	u8 charId[TAG_CHAR_ID_LENGTH];
	res = tag_charIdDataFromTag(data, sizeof(data), charId, sizeof(charId));
	if (res != TAG_ERR_OK) {
		snprintf(tagName, sizeof(tagName), "%02X%02X%02X%02X%02X%02X%02X", data[0], data[1], data[2], data[3], data[4], data[5], data[6]); //uid
	} else {
		struct AmiiboIdStruct *charinfo = parseCharData(charId);

		if (!getNameByAmiiboId(charinfo->amiiboId, tagName, sizeof(tagName))) {
			snprintf(tagName, sizeof(tagName), "%02X%02X%02X%02X%02X%02X%02X", data[0], data[1], data[2], data[3], data[4], data[5], data[6]); //uid
			cleanFilename(tagName);
		}
	}

	time_t unixTime = time(NULL);
	struct tm* timestruct = gmtime((const time_t *)&unixTime);
	int hours = timestruct->tm_hour;
	int minutes = timestruct->tm_min;
	int day = timestruct->tm_mday;
	int month = timestruct->tm_mon +1;
	int year = timestruct->tm_year +1900;

	char dumpFileName[200];
	snprintf(dumpFileName, sizeof(dumpFileName),
		"%s/%s_%02d%02d%02d%02d%02d.bin",
		AMIIBO_DUMP_ROOT,
		tagName,
		year, month, day, hours, minutes);
	uiUpdateStatus("Writing to file..");
	printf("Writing to file %s\n", dumpFileName);
	res = writeFile(dumpFileName, data, sizeof(data));
	if (res <0) {
		printf("Write to disk failed: %d\n", res);
		goto dumpTagToFile_ERROR;
	}
	uiUpdateStatus("");
	uiUpdateProgress(0, -1);
	uiSelectMain();
	printf("\e[2J\e[H\e[0m\e[5;2HWrote to file:\n  %s\n\n   Press A to continue.", dumpFileName);
	uiGetKey(KEY_A);
	return;
dumpTagToFile_ERROR:
	uiUpdateStatus("ERROR");
	uiSelectMain();
	printf("\e[2J\e[H\e[0m\e[5;2HSave tag to file failed.\n\n   Press A to continue.");
	uiGetKey(KEY_A);
	uiUpdateStatus("");
	uiUpdateProgress(0, -1);
}

void uiShowTagInfo() {
	u8 charId[TAG_CHAR_ID_LENGTH];
	int res = tag_getCharIdData(charId, sizeof(charId));
	if (res != TAG_ERR_OK) {
		printf("Could not read character id: %02x\n", res);
		return;
	}
	u8 uid7[TAG_UID7_LENGTH];
	res = tag_getUid7(uid7, sizeof(uid7));
	if (res != TAG_ERR_OK) {
		printf("Could not read uid id: %02x\n", res);
		return;
	}

	struct AmiiboIdStruct *charinfo = parseCharData(charId);

	char name[1024];
	if (!getNameByAmiiboId(charinfo->amiiboId, name, sizeof(name))) {
		printf("%0x\n", charinfo->amiiboId);
		snprintf(name, sizeof(name), "Unknown (%0x%0x%0x%0x%0x%0x%0x%0x)", charId[0], charId[1], charId[2], charId[3], charId[4], charId[5], charId[6], charId[7]);
	}

	uiSelectMain();
	printf("\e[0m\e[5;2HType : \e[1m%s", name);
	printf("\e[0m\e[7;2HUID  : \e[1m%02x%02x%02x%02x%02x%02x%02x", uid7[0], uid7[1], uid7[2], uid7[3], uid7[4], uid7[5], uid7[6]);

	return;
}

u32 showMenu() {
	uiSelectMain();
	uiClearScreen();
	printf("\e[2;1H X - Load tag from file.");
	if (tag_isLoaded())
		printf("\e[3;1H A - Write/Restore Tag.");
	else
		printf("\e[3;1H A - N2 Elite options.");
	printf("\e[2;26H Y - Dump Tag to file.");
	printf("\e[3;26H B - Quit.");
	uiSelectLog();

	if (tag_isLoaded()) {
		uiShowTagInfo();
	}
	return uiGetKey(KEY_X | KEY_A | KEY_Y | KEY_B);
}

void menu() {
	while (aptMainLoop()) {
		u32 kDown = showMenu();

		if (kDown & KEY_X) {
			loadDump();
		} else if ((kDown & KEY_A)) {
			if (tag_isLoaded() && tag_isKeysLoaded()) {
				writeToTag();
			} else {
				menuElite();
			}
		} else if (kDown & KEY_Y) {
			dumpTagToFile();
		} else if (kDown & KEY_B)
			break;
	}
}

int main() {
	uiInit();

	uiUpdateBanner();
	uiInitStatus();

	gfxFlushBuffers();
	gfxSwapBuffers();
	gspWaitForVBlank();

	if (loadKeys()) {
		if (nfc_init()) {
			menu();
			nfc_exit();
		}
	}

	uiUpdateProgress(0, -1);
	uiSelectMain();
	printf("\e[2J\e[H\e[8;15HPress any key to exit.");

	uiGetKey(0xFFFFFF);

	#if 0 //reboot
	aptInit();
	APT_HardwareResetAsync();
	aptExit();
	#endif

	uiExit();
	return 0;
}