#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <3ds.h>
#include <3ds/errf.h>
#include <3ds/services/nfc.h>
#include <3ds/services/apt.h>

#include "nfc.h"
#include "filepicker.h"
#include "nfc3d/amitool.h"

#define NFC_TIMEOUT  200 * 1000000

#define CMD_FAST_READ(pagestart, pagecount) {0x3A, pagestart, pagestart+pagecount-1}
#define CMD_READ(pagestart) {0x30, pagestart}
#define CMD_WRITE(pagestart, data) {0xA2, pagestart, data[0], data[1], data[2], data[3]}

#define NTAG_215_LAST_PAGE 0x86

static void printbuf(char *prefix, u8* data, size_t len) {
	char bufstr[len*3 + 3];
	memset(bufstr, 0, sizeof(bufstr));
	for(int pos=0; pos<len; pos++) {
		snprintf(&bufstr[pos*3], 4, "%02x ", data[pos]);
		if (pos > 0 && pos % 12 == 0) {
			bufstr[pos*3+2] = '\n';
		}
	}
	printf("%s hex: %s\n", prefix, bufstr);
}

#if 1

#define DnfcStartOtherTagScanning nfcStartOtherTagScanning
#define DnfcGetTagState nfcGetTagState
#define DnfcSendTagCommand nfcSendTagCommand
#define DnfcSendTagCommand nfcSendTagCommand
#define DnfcStopScanning nfcStopScanning

#else
static Result DnfcStartOtherTagScanning(int a, int b) {
	return 0;
}

static Result DnfcGetTagState(NFC_TagState *state) {
	*state = NFC_TagState_InRange;
	return 0;
}

static Result DnfcSendTagCommand(u8* command, int cmdlen, u8* dest, int destlen, size_t *readsize, u64 timeout) {
	printbuf("cmd>", command, cmdlen);
	static int i = 0;
	for(int x = 0; x<destlen; x++) {
		if (x % 4 == 0) i++;
		dest[x] = i;
	}
	*readsize = destlen;
	return 0;
}

static void DnfcStopScanning() {
	
}
#endif

Result nfc_readFull(u8 *data, int datalen) {
	if (datalen < NTAG_PAGE_SIZE * 4) {
		return -1;
	}
	Result ret=0;

	NFC_TagState prevstate, curstate;

	ret = DnfcStartOtherTagScanning(NFC_STARTSCAN_DEFAULTINPUT, 0x01);
	if(R_FAILED(ret)) {
		printf("StartOtherTagScanning() failed.\n");
		return ret;
	}

	prevstate = NFC_TagState_Uninitialized;
	while (1) {
		gspWaitForVBlank();
		hidScanInput();

		u32 kDown = hidKeysDown();
		
		if(kDown & KEY_B) break;
		
		ret = DnfcGetTagState(&curstate);
		if(R_FAILED(ret)) {
			printf("nfcGetTagState() failed.\n");
			DnfcStopScanning();
			return ret;
		}
		
		if(curstate!=prevstate) {
			prevstate = curstate;
			if(curstate==NFC_TagState_InRange) {
				u8 tagdata[AMIIBO_MAX_SIZE];
				memset(tagdata, 0, sizeof(tagdata));
				
				for(int i=0x00; i<=NTAG_215_LAST_PAGE ;i+=NTAG_FAST_READ_PAGE_COUNT) {
					printf("Page %d\n", i);
					u8 cmd[] = CMD_FAST_READ(i, NTAG_FAST_READ_PAGE_COUNT);
					u8 cmdresult[NTAG_FAST_READ_PAGE_COUNT*NTAG_PAGE_SIZE];
					printbuf("CMD ", cmd, sizeof(cmd));
					memset(cmdresult, 0, sizeof(cmdresult));
					size_t resultsize = 0;
					
					printf("page start %x end %x\n", cmd[1], cmd[2]);
					resultsize = NTAG_FAST_READ_PAGE_COUNT*NTAG_PAGE_SIZE;
					
					ret = DnfcSendTagCommand(cmd, sizeof(cmd), cmdresult, sizeof(cmdresult), &resultsize, NFC_TIMEOUT);
					if(R_FAILED(ret)) {
						printf("nfcSendTagCommand() failed : %ld\n", R_DESCRIPTION(ret));
						break;
					}
					if (resultsize < NTAG_FAST_READ_PAGE_COUNT * NTAG_PAGE_SIZE) {
						printf("Read size mismatch expected %d got %d.\n", NTAG_FAST_READ_PAGE_COUNT * NTAG_PAGE_SIZE, resultsize);
						ret = -1;
						break;
					}

					int copycount = sizeof(cmdresult);
					if (((i * NTAG_PAGE_SIZE) + sizeof(cmdresult)) > sizeof(tagdata))
						copycount = ((i * NTAG_PAGE_SIZE) + sizeof(cmdresult)) - sizeof(tagdata);
					printf(">%d ", copycount);
					memcpy(&tagdata[i * NTAG_PAGE_SIZE], cmdresult, copycount);
					printbuf("result", cmdresult, resultsize); 
				}
				if (ret == 0) {
					memcpy(data, tagdata, sizeof(tagdata));
					//printbuf("result full ", tagdata, AMIIBO_MAX_SIZE);
				}
				break;
			}
		}
	}
	
	DnfcStopScanning();
	return ret;
}

Result nfc_readBlock(int pageId, u8 *data, int datalen) {
	if (datalen < NTAG_BLOCK_SIZE) {
		return -1;
	}
	if (pageId < 0 || pageId > NTAG_215_LAST_PAGE)
		return -1;
	
	Result ret=0;
	
	memset(data, 0, datalen);

	NFC_TagState prevstate, curstate;

	ret = DnfcStartOtherTagScanning(NFC_STARTSCAN_DEFAULTINPUT, 0x01);
	if(R_FAILED(ret)) {
		printf("StartOtherTagScanning() failed.\n");
		return ret;
	}

	prevstate = NFC_TagState_Uninitialized;
	while (1) {
		gspWaitForVBlank();
		hidScanInput();

		u32 kDown = hidKeysDown();
		
		if(kDown & KEY_B) break;
		
		ret = DnfcGetTagState(&curstate);
		if(R_FAILED(ret))
		{
			printf("nfcGetTagState() failed.\n");
			DnfcStopScanning();
			return ret;
		}
		
		if(curstate!=prevstate) {
			prevstate = curstate;
			if(curstate==NFC_TagState_InRange) {
				u8 cmd[] = CMD_READ(pageId);
				size_t resultsize = 0;
				ret = DnfcSendTagCommand(cmd, sizeof(cmd), data, datalen, &resultsize, NFC_TIMEOUT);
				if(R_FAILED(ret)) {
					printf("nfcSendTagCommand() failed : %ld\n", R_DESCRIPTION(ret));
					break;
				}
				if (resultsize < NTAG_BLOCK_SIZE) {
					printf("Read size mismatch expected %d got %d.\n", NTAG_BLOCK_SIZE, resultsize);
					ret = -1;
					break;
				}
				break;
			}
		}
	}
	
	DnfcStopScanning();
	return ret;
}
/*

static Result writePage(int pageId, u8 data) {
	u8 cmd[] = CMD_WRITE(pageId, data);
	size_t resultsize = 0;
	ret = DnfcSendTagCommand(cmd, sizeof(cmd), data, datalen, &resultsize, NFC_TIMEOUT);
	if(R_FAILED(ret)) {
		printf("Writing Tag page %d failed : %ld\n", pageId, R_DESCRIPTION(ret));
	}
	return ret;
}

static Result writeTag(u8 *data, u8 *PWD, u8 *PACK) {
	//write normal pages
	for(int pageid = 0x04; pageid <= 0x81; pageid++) {
		ret = writePage(pageid, &data[pageId * NTAG_PAGE_SIZE]);
		if (R_FAILED(ret))
			return ret;
	}
	
	//write OTP
	
	//write PACK
	
	//write PWD
	
	//write LOCK
	
	return 0;
}

Result nfc_write(u8 *data, int datalen, u8 *PWD, int PWDLength) {
	if (datalen < NTAG_PAGE_SIZE * 0x81) return -1;
	if (PWDLength != NTAG_PAGE_SIZE) return -1;
	
	Result ret=0;
	
	NFC_TagState prevstate, curstate;

	ret = DnfcStartOtherTagScanning(NFC_STARTSCAN_DEFAULTINPUT, 0x01);
	if(R_FAILED(ret)) {
		printf("StartOtherTagScanning() failed.\n");
		return ret;
	}

	prevstate = NFC_TagState_Uninitialized;
	while (1) {
		gspWaitForVBlank();
		hidScanInput();

		u32 kDown = hidKeysDown();
		
		if(kDown & KEY_B) break;
		
		ret = DnfcGetTagState(&curstate);
		if(R_FAILED(ret)) {
			printf("nfcGetTagState() failed.\n");
			DnfcStopScanning();
			return ret;
		}
		
		if(curstate!=prevstate) {
			prevstate = curstate;
			if(curstate==NFC_TagState_InRange) {
				ret = writeTag(daa, PWD, PACK)
				break;
			}
		}
	}
	
	DnfcStopScanning();
	return ret;
}
*/