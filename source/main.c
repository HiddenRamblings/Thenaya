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

void printbuf(char *prefix, u8* data, size_t len);

Result nfc_test()
{
	Result ret=0;

	NFC_TagState prevstate, curstate;
	NFC_TagInfo taginfo;

	printf(__TIME__);
	printf("NFC initialized successfully, starting scanning...\n");

	/*
	0x0 - crash
	0x1 - detects as normal
	0x2 - not detect
	0x4 - not detect
	*/
	
	ret = nfcStartOtherTagScanning(NFC_STARTSCAN_DEFAULTINPUT, 0x01);
	if(R_FAILED(ret))
	{
		printf("StartOtherTagScanning() failed.\n");
		return ret;
	}

	ret = nfcGetTagState(&curstate);
	if(R_FAILED(ret))
	{
		printf("nfcGetTagState() failed.\n");
		nfcStopScanning();
		return ret;
	}

	prevstate = curstate;

	printf("B - Quit, Y - Restart Scanning.\n");
	printf("X - Restart Scan while in range.\n");
	printf("Hold down A while processing an amiibo to write\nappdata from SD to it. The amiibo appdata will be\ninitialized if it wasn't initialized already.\n");
	//printf("Scanning must be restarted after a NFC tag goes\nout-of-range, if you want to scan more tag(s).\n");

	while(1)
	{
		gspWaitForVBlank();
		hidScanInput();

		u32 kDown = hidKeysDown();

		if(kDown & KEY_B)break;

		if(kDown & KEY_Y)//If you want this to be done automatically, you could run this when the TagState changes to NFC_TagState_OutOfRange.
		{
			printf("Restarting scanning...\n");

			nfcStopScanning();
			ret = nfcStartOtherTagScanning(NFC_STARTSCAN_DEFAULTINPUT, 0x01);
			if(R_FAILED(ret))
			{
				printf("nfcStartScanning() failed.\n");
				return ret;
			}

			printf("Scanning restarted.\n");

			continue;
		}

		if(kDown & KEY_X)
		{
			ret = nfcResetTagScanState();
			if(R_FAILED(ret))
			{
				printf("nfcResetTagScanState() failed.\n");
				break;
			}

			printf("nfcResetTagScanState() was successful.\n");
		}

		ret = nfcGetTagState(&curstate);
		if(R_FAILED(ret))
		{
			printf("nfcGetTagState() failed.\n");
			break;
		}

		if(curstate!=prevstate)//See nfc.h for the TagState values.
		{
			printf("TagState changed from %d to %d.\n", prevstate, curstate);
			prevstate = curstate;

			if(curstate==NFC_TagState_InRange)
			{
				memset(&taginfo, 0, sizeof(NFC_TagInfo));

				/*
				ret = nfcGetTagInfo(&taginfo);
				if(R_FAILED(ret))
				{
					printf("nfcGetTagInfo() failed.\n");
					break;
				}*/

				printf("taginfo id_offset_size: 0x%x.\n", (unsigned int)taginfo.id_offset_size);
				printf("NFC tag UID:\n");
				printbuf("uidstr", taginfo.id, 7);
				
				/*
				ret = nfcCmd22();
				if(R_FAILED(ret))
				{
					printf("nfcCmd21() failed.\n");
					printf("R : %ld\n", R_DESCRIPTION(ret));
				}
				*/
				
				#if 0
				u8 cmd[2];
				u8 cmdresult[100];
				memset(cmdresult, 0, sizeof(cmdresult));
				size_t resultsize = 0;
				cmd[0] = 0x30; //READ
				cmd[1] = 0x02; //page 3
				printbuf("cmd", cmd, sizeof(cmd));
				ret = nfcSendTagCommand(cmd, 2, cmdresult, sizeof(cmdresult), &resultsize, 200 * 1000000);
				printf("Result size : %d\n", resultsize);
				printf("R : %ld %ld\n", R_DESCRIPTION(ret), R_MODULE(ret));
				
				printbuf("result", cmdresult, 16);
				if(R_FAILED(ret))
				{
					printf("nfcSendTagCommand() failed.\n");
					ERRF_ThrowResultWithMessage(ret ,"bla bla");
					break;
				}
				#endif
				
				#if 0
				u8 cmd[3];
				u8 cmdresult[600];
				memset(cmdresult, 0, sizeof(cmdresult));
				size_t resultsize = 0;
				cmd[0] = 0x3A; //FAST READ
				cmd[1] = 0x00; //page 3
				cmd[2] = 0x86; //page 3
				ret = nfcSendTagCommand(cmd, 3, cmdresult, sizeof(cmdresult), &resultsize, 200 * 1000000);
				printf("Result size : %d\n", resultsize);
				printf("R : %ld %ld\n", R_DESCRIPTION(ret), R_MODULE(ret));
				
				printbuf("result", cmdresult, resultsize);
				if(R_FAILED(ret))
				{
					printf("nfcSendTagCommand() failed.\n");
					break;
				}
				#endif
				
				#if 1
				u8 cmd[6];
				u8 cmdresult[100];
				size_t resultsize = 0;
				cmd[0] = 0xA2; //WRITE
				cmd[1] = 0x05; //page 5
				cmd[2] = 0x0d;
				cmd[3] = 0x0e;
				cmd[4] = 0x0a;
				cmd[5] = 0x09;
				printbuf("cmd", cmd, sizeof(cmd));
				ret = nfcSendTagCommand(cmd, sizeof(cmd), cmdresult, sizeof(cmdresult), &resultsize, 200 * 1000000);
				printf("R : %d\n", R_DESCRIPTION(resultsize));
				printbuf("result data", cmdresult, resultsize);
				if(R_FAILED(ret))
				{
					printf("nfcSendTagCommand() failed.\n");
					break;
				}
				printf("result size %d.\n", resultsize);
				#endif
				
				printf("amiibo NFC tag processing has finished, you can\nremove the tag from NFC-range now.\n");
			}
		}
	}

	nfcStopScanning();

	return ret;
}

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

#define KEY_FILE_PATH "sdmc:/amiibo_keys.bin"
#define KEY_FILE_SIZE 160

#define AMIIBO_FILE "sdmc:/linkarcheramiibo.bin"

#define AMIIBO_DUMP_ROOT "sdmc:/amiibo"

int loadKeys() {
	u8 keybuffer[KEY_FILE_SIZE];
	printf("Reading key file from %s\n", KEY_FILE_PATH);
	int size = readFile(KEY_FILE_PATH, keybuffer, KEY_FILE_SIZE);
	if (size < 0) {
		printf("Failed to read key file: %d\n", size);
		return 0;
	}
	if (size != KEY_FILE_SIZE) {
		printf("Failed to read key file (file size incorrect: %d)\n", size);
		return 0;
	}
	
	if (tag_setKeys(keybuffer, size) != 0) {
		printf("Invalid key file");
		return 0;
	}
	
	printf("Key file loaded.\n");
	return 1;
}

void loadDump() {
	char filename[PICK_FILE_SIZE];
	if (!pick_file(filename, AMIIBO_DUMP_ROOT)) {
		printf("No file selected\n");
		return;
	}
	printf("File selected %s\n", filename);
	u8 tagdata[AMIIBO_MAX_SIZE];
	int size = readFile(filename, tagdata, AMIIBO_MAX_SIZE);
	if (size < 0) {
		printf("Failed to read key file: %d\n", size);
		return;
	}
	
	printbuf("orginial uid : ", tagdata, 9);
	
	int res = tag_setTag(tagdata, size);
	if (res != TAG_ERR_OK) {
		printf("Failed to load tag: %d\n", res);
		return;
	}
	printf("Tag dump loaded.\n");
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
	printf("Place tag on scanner...\n");
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
	printf("Place tag on scanner...\n");
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

void menu() {
	printf("X - Load tag dump.\n");
	printf("A - Write/Restore to tag.\n");
	printf("Y - Dump tag to file.\n");
	printf("B - Quit\n");
	
	while (aptMainLoop()) {
		gspWaitForVBlank();
		hidScanInput();
		u32 kDown = hidKeysDown();
		
		if (kDown & KEY_X)
			loadDump();
		else if (kDown & KEY_A)
			writeToTag();
		else if (kDown & KEY_Y)
			dumpTagToFile();
		else if (kDown & KEY_B)
			break;
	}
}

void start() {
	if (!loadKeys())
		return;
	Result ret = nfcInit(NFC_OpType_RawNFC);
	if(R_FAILED(ret)) {
		printf("nfcInit() failed: 0x%08x.\n", (unsigned int)ret);
		return;
	}
		
	menu();
	nfcExit();
}

int main() {
	gfxInitDefault();
	consoleInit(GFX_TOP, NULL);

	printf("Thenaya - Amiibo Maker v0.1 (Alpha)\n");
	
	start();

	printf("Press START to exit.\n");

	// wait till START
	while (aptMainLoop()) {
		gspWaitForVBlank();
		hidScanInput();

		u32 kDown = hidKeysDown();

		if (kDown & KEY_START)
			break;
	}
	
	//reboot
	//https://github.com/AlbertoSONIC/3DS_Quick_Reboot/blob/master/source/main.c
	aptInit();
	APT_HardwareResetAsync();
	aptExit();

	gfxExit();
	return 0;
}

