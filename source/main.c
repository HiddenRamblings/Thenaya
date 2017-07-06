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
#include "filepicker.h"
#include "nfc3d/amitool.h"
#include "ui.h"

#define KEY_FILE_PATH "sdmc:/amiibo_keys.bin"
#define KEY_FILE_SIZE 160

//#define AMIIBO_DUMP_ROOT "sdmc:/amiibo"
#define AMIIBO_DUMP_ROOT "sdmc:/"


void printbuf(char *prefix, u8* data, size_t len);
void uiShowTagInfo(u8 *charId, u8 *uid);

void printbuf(char *prefix, u8* data, size_t len) {
	char bufstr[len*2 + 2];
	memset(bufstr, 0, sizeof(bufstr));
	for(int pos=0; pos<len; pos++)snprintf(&bufstr[pos*2], 3, "%02x", data[pos]);
	printf("%s hex: %s\n", prefix, bufstr);
}

int readFile(char *filepath, u8 *data, u32 datasize) {
	struct stat filestats;
	FILE *f;
	int readsize=0;

	if(stat(filepath, &filestats)==-1) return -1;

	if (filestats.st_size > datasize) {
		printf("File too large: 0x%08x.\n", (unsigned int)filestats.st_size);
		return -2;
	}
	
	f = fopen(filepath, "r");
	if(f==NULL) return -3;

	readsize = fread(data, 1, filestats.st_size, f);
	fclose(f);

	if(readsize!=filestats.st_size) return -4;

	return readsize;
}

int writeFile(char *filepath, u8 *data, u32 datasize) {
	FILE *f;
	int writesize=0;

	f = fopen(filepath, "w");
	if(f==NULL) return -3;

	writesize = fwrite(data, 1, datasize, f);
	fclose(f);

	if(writesize!=datasize) return -4;

	return writesize;
}

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
	
	u8 charId[TAG_CHAR_ID_LENGTH];
	int res = tag_charIdDataFromTag(tagdata, size, charId, sizeof(charId));
	if (res != TAG_ERR_OK) {
		printf("Could not read character id: %0x\n", res);
		goto END_loadDump;
	}
	
	res = tag_setTag(tagdata, size);
	if (res != TAG_ERR_OK) {
		printf("Failed to load tag: %d\n", res);
		goto END_loadDump;
	}
	
	uiShowTagInfo(charId, tagdata);
	
END_loadDump:
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
	printf("Place tag on scanner, or press B to cancel.\n");
	u8 firstPages[NTAG_BLOCK_SIZE];
	
	int res = nfc_readBlock(0, firstPages, sizeof(firstPages));
	if (res != 0) {
		printf("Failed to get UID: %d\n", res);
		return;
	}
	printf("Got new UID\n");
	res = tag_setUid(firstPages, 9);
	if (res != TAG_ERR_OK) {
		printf("Failed to update UID: %d\n", res);
		return;
	}
	printf("Encrypting...\n");
	u8 data[AMIIBO_MAX_SIZE];
	res = tag_getTag(data, sizeof(data));
	if (res != TAG_ERR_OK) {
		printf("Failed to encrypt tag: %d\n", res);
		return;
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
		return;
	}
	
	printf("Calculating password...\n");
	u8 pwd[NTAG_PAGE_SIZE];
	res = tag_calculatePassword(uid, sizeof(uid), pwd, sizeof(pwd));
	if (res != TAG_ERR_OK) {
		printf("Failed to calculate pwd: %d\n", res);
		return;
	}
	
	printf("Writing tag...\n");
	res = nfc_write(data, sizeof(data), pwd, sizeof(pwd));
	if (res != 0) {
		printf("nfc write failed %d\n", res);
	}
	printf("finished\n");
}

void dumpTagToFile() {
	printf("Place tag on scanner, or press B to cancel\n");
	u8 data[AMIIBO_MAX_SIZE];
	int res = nfc_readFull(data, sizeof(data));
	if (res != 0) {
		printf("Scanning failed\n");
		return;
	}
	printbuf("UID ", data, 8);
	
	mkdir(AMIIBO_DUMP_ROOT, 0777);
	
	time_t unixTime = time(NULL);
	struct tm* timestruct = gmtime((const time_t *)&unixTime);
	int hours = timestruct->tm_hour;
	int minutes = timestruct->tm_min;
	int day = timestruct->tm_mday;
	int month = timestruct->tm_mon;
	int year = timestruct->tm_year +1900;
		
	char dumpFileName[200];
	snprintf(dumpFileName, sizeof(dumpFileName),
		"%s/%02X%02X%02X%02X%02X%02X%02X_%02d%02d%02d%02d%02d.bin",
		AMIIBO_DUMP_ROOT, 
		data[0], data[1], data[2], data[3], data[4], data[5], data[6], //uid
		year, month, day, hours, minutes);
	printf("Writing to file %s\n", dumpFileName);
	res = writeFile(dumpFileName, data, sizeof(data));
	if (res <0)
		printf("Write to disk failed: %d\n", res);
	printf("Finished\n");
}

u32 showMenu() {
	uiSelectMain();
	uiClearScreen();
	printf("\e[1;0H X - Load tag from file.");
	if (tag_isLoaded())
		printf("\e[2;0H A - Write/Restore Tag.");
	printf("\e[1;26H Y - Dump Tag to file.");
	printf("\e[2;26H B - Quit.");
	uiSelectLog();
	return uiGetKey(KEY_X | KEY_A | KEY_Y | KEY_B);
}

void menu() {
	while (aptMainLoop()) {
		u32 kDown = showMenu();
		
		if (kDown & KEY_X) {
			loadDump();
		} else if ((kDown & KEY_A) && tag_isLoaded() && tag_isKeysLoaded()) {
			writeToTag();
		} else if (kDown & KEY_Y) {
			dumpTagToFile();
		} else if (kDown & KEY_B)
			break;
	}
}

void uiShowTagInfo(u8 *charId, u8 *uid) {
	printbuf("orginial uid : ", uid, TAG_UID_LENGTH);
	printbuf("Char Id: ", charId, TAG_CHAR_ID_LENGTH);
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
	printf("\e[2J\e[H\x1b[7;15HPress Start to exit.");

	uiGetKey(KEY_START);
	
	#if 0 //reboot
	aptInit();
	APT_HardwareResetAsync();
	aptExit();
	#endif

	uiExit();
	return 0;
}