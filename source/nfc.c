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
#include "ui.h"
#include "util2.h"

#define NFC_EMULATE 0

#define NFC_TIMEOUT  200 * 1000000

#define CMD_FAST_READ(pagestart, pagecount) {0x3A, pagestart, pagestart+pagecount-1}
#define CMD_READ(pagestart) {0x30, pagestart}
#define CMD_WRITE(pagestart, data) {0xA2, pagestart, data[0], data[1], data[2], data[3]}
#define CMD_AUTH(pwd) {0x1B, pwd[0], pwd[1], pwd[2], pwd[3]}

#define NTAG_PACK {0x80, 0x80, 0x00, 0x00}

#define NTAG_215_LAST_PAGE 0x86

#if !NFC_EMULATE

#define DnfcStartOtherTagScanning nfcStartOtherTagScanning
#define DnfcGetTagState nfcGetTagState
#define DnfcSendTagCommand nfcSendTagCommand
#define DnfcSendTagCommand nfcSendTagCommand
#define DnfcStopScanning nfcStopScanning

#else
	
//emulate the calls for running in citra
static Result DnfcStartOtherTagScanning(int a, int b) {
	return 0;
}

static Result DnfcGetTagState(NFC_TagState *state) {
	*state = NFC_TagState_InRange;
	return 0;
}

static Result DnfcSendTagCommand(u8* command, int cmdlen, u8* dest, int destlen, size_t *readsize, u64 timeout) {
	//printbuf("cmd>", command, cmdlen);
	svcSleepThread(200000000);
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
	#if NFC_EMULATE
	
	readFile("sdmc:/linkarcheramiibo.bin", data, datalen);
	
	return 0;
	
	#endif
	
	if (datalen < NTAG_PAGE_SIZE * 4) {
		return -1;
	}
	Result ret=0;

	NFC_TagState prevstate, curstate;

	ret = DnfcStartOtherTagScanning(NFC_STARTSCAN_DEFAULTINPUT, 0x01);
	if(R_FAILED(ret)) {
		printf("StartOtherTagScanning() failed: 0x%08x.\n", (unsigned int)ret);
		return ret;
	}

	prevstate = NFC_TagState_Uninitialized;
	while (1) {
		gspWaitForVBlank();
		hidScanInput();

		u32 kDown = hidKeysDown();
		
		if(kDown & KEY_B) {
			ret = -1;
			printf("Cancelled.\n");
			break;
		}
		
		ret = DnfcGetTagState(&curstate);
		if(R_FAILED(ret)) {
			printf("nfcGetTagState() failed: 0x%08x.\n", (unsigned int)ret);
			DnfcStopScanning();
			return ret;
		}
		
		if(curstate!=prevstate) {
			prevstate = curstate;
			if(curstate==NFC_TagState_InRange) {
				uiUpdateStatus("Tag detected.");
				u8 tagdata[AMIIBO_MAX_SIZE];
				memset(tagdata, 0, sizeof(tagdata));
				
				printf("Reading tag");
				for(int i=0x00; i<=NTAG_215_LAST_PAGE ;i+=NTAG_FAST_READ_PAGE_COUNT) {
					uiUpdateStatus("Reading.");
					uiUpdateProgress(i, NTAG_215_LAST_PAGE);
					u8 cmd[] = CMD_FAST_READ(i, NTAG_FAST_READ_PAGE_COUNT);
					u8 cmdresult[NTAG_FAST_READ_PAGE_COUNT*NTAG_PAGE_SIZE];
					//printbuf("CMD ", cmd, sizeof(cmd));
					memset(cmdresult, 0, sizeof(cmdresult));
					size_t resultsize = 0;
					
					//printf("page start %x end %x\n", cmd[1], cmd[2]);
					resultsize = NTAG_FAST_READ_PAGE_COUNT*NTAG_PAGE_SIZE;
					
					ret = DnfcSendTagCommand(cmd, sizeof(cmd), cmdresult, sizeof(cmdresult), &resultsize, NFC_TIMEOUT);
					if(R_FAILED(ret)) {
						printf("nfcSendTagCommand() failed: 0x%08x.\n", (unsigned int)ret);
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
					memcpy(&tagdata[i * NTAG_PAGE_SIZE], cmdresult, copycount);
					//printbuf("result", cmdresult, resultsize); 
				}
				if (ret == 0) {
					memcpy(data, tagdata, sizeof(tagdata));
					//printbuf("result full ", tagdata, AMIIBO_MAX_SIZE);
				}
				break;
			}
		}
	}
	uiUpdateProgress(0, -1);
	printf("\n");
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
		
		if(kDown & KEY_B) {
			ret = -1;
			printf("Cancelled.\n");
			break;
		}
		
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
					printf("nfcSendTagCommand() failed: 0x%08x.\n", (unsigned int)ret);
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

static Result writePage(int pageId, u8 *data) {
	//debug code:
	//pageId = 0x06;
	u8 cmd[] = CMD_WRITE(pageId, data);
	size_t resultsize = 0;
	u8 buffer[100];
	int ret = DnfcSendTagCommand(cmd, sizeof(cmd), buffer, sizeof(buffer), &resultsize, NFC_TIMEOUT);
	if(R_FAILED(ret)) {
		printf("Writing Tag page %d failed: 0x%08x.\n", pageId, (unsigned int)ret);
	} else if (resultsize >=1 && buffer[0] != 0x0A)
		printf("write page returned a NAK %d\n", buffer[0]);
	return ret;
}

static Result writeTag(u8 *data, u8 *PWD, u8 *PACK) {
	//write normal pages
	int ret = 0;
	
	int stepCount = 132;
	int step = 0;
	uiUpdateStatus("Writing normal pages");
	uiUpdateProgress(step++, stepCount);
	printf("Writing normal pages\n");
	for(int pageId = 0x04; pageId <= 0x81; pageId++) {
		uiUpdateProgress(step++, stepCount);
		ret = writePage(pageId, &data[pageId * NTAG_PAGE_SIZE]);
		if (R_FAILED(ret))
			return ret;
	}
	printf("\n");
	
	//write OTP
	printf("Writing OTP\n");
	uiUpdateStatus("Writing OTP");
	uiUpdateProgress(step++, stepCount);
	ret = writePage(0x03, &data[0x03 * NTAG_PAGE_SIZE]);
	if (R_FAILED(ret))
		return ret;

	ret = nfcCmd21(); //seems to put the NFC reader into a continious mode allowing all the requests to go through one session without powering the tag down.
	if(R_FAILED(ret)) {
		printf("nfcCmd21 failed: 0x%08x.\n", (unsigned int)ret);
		return ret;
	}

	//write PACK
	printf("Writing PACK\n");
	uiUpdateStatus("Writing PACK");
	uiUpdateProgress(step++, stepCount);
	ret = writePage(0x86, PACK);
	if (R_FAILED(ret))
		return ret;
	
	//write PWD
	printf("Writing PWD\n");
	uiUpdateStatus("Writing PWD");
	ret = writePage(0x85, PWD);
	uiUpdateProgress(step++, stepCount);
	if (R_FAILED(ret))
		return ret;
	
	uiUpdateStatus("Writing lock bits");
	//write LOCK
	printf("Writing lock bits");
    
	printf(".");
	uiUpdateProgress(step++, stepCount);
	//tag.writePage(2, new byte[]{pages[2 * TagUtil.PAGE_SIZE], pages[(2 * TagUtil.PAGE_SIZE) + 1], (byte) 0x0F, (byte) 0xE0}); //lock bits	
	ret = writePage(0x02, &data[0x02 * NTAG_PAGE_SIZE]); //static lock bits
	if (R_FAILED(ret))
		return ret;
	
	printf(".");
	uiUpdateProgress(step++, stepCount);
	u8 dynamiclock[] = {0x01, 0x00, 0x0F, 0x00}; //dynamic lock bits.
	ret = writePage(0x82, dynamiclock);
	if (R_FAILED(ret))
		return ret;

	printf(".");
	uiUpdateProgress(step++, stepCount);
	u8 config1[] = {0x00, 0x00, 0x00, 0x04}; //config
	ret = writePage(0x83, config1);
	if (R_FAILED(ret))
		return ret;
	
	printf(".");
	uiUpdateProgress(step++, stepCount);
	u8 config2[] = {0x5F, 0x00, 0x00, 0x00}; //config
	ret = writePage(0x84, config2);
	if (R_FAILED(ret))
		return ret;
	
	printf("\n");

	ret = nfcCmd22(); //power down the tag
	if(R_FAILED(ret)) {
		printf("nfcCmd22 failed: 0x%08x.\n", (unsigned int)ret);
		return ret;
	}

	return 0;
}

Result nfc_auth(u8 *PWD) {
	u8 pwdcmd[] = CMD_AUTH(PWD); //auth
	size_t resultsize = 0;
	u8 packres[2];
	int ret = DnfcSendTagCommand(pwdcmd, sizeof(pwdcmd), packres, sizeof(packres), &resultsize, NFC_TIMEOUT);
	//printbuf("pack ", packres, sizeof(packres));
	if(R_FAILED(ret)) {
		printf("PWD command failed: 0x%08x.\n", (unsigned int)ret);
		return ret;
	}
	
	if (!((packres[0] == packres[1]) && (packres[0] == 0x80))) {
		printf("PWD auth failed resp %x %x", packres[0], packres[1]);
		return -1;
	}
	
	return 0;
}

Result nfc_write(u8 *data, int datalen, u8 *PWD, int PWDLength) {
	if (datalen < NTAG_PAGE_SIZE * 0x81) return -1;
	if (PWDLength != NTAG_PAGE_SIZE) return -1;
	
	Result ret=0;
	
	NFC_TagState prevstate, curstate;

	ret = DnfcStartOtherTagScanning(NFC_STARTSCAN_DEFAULTINPUT, 0x01);
	if(R_FAILED(ret)) {
		printf("StartOtherTagScanning() failed: 0x%08x.\n", (unsigned int)ret);
		return ret;
	}

	prevstate = NFC_TagState_Uninitialized;
	while (1) {
		gspWaitForVBlank();
		hidScanInput();

		u32 kDown = hidKeysDown();
		
		if(kDown & KEY_B) {
			ret = -1;
			printf("Cancelled.\n");
			break;
		}
		
		ret = DnfcGetTagState(&curstate);
		if(R_FAILED(ret)) {
			printf("nfcGetTagState() failed: 0x%08x.\n", (unsigned int)ret);
			DnfcStopScanning();
			return ret;
		}
		
		if(curstate!=prevstate) {
			prevstate = curstate;
			if(curstate==NFC_TagState_InRange) {
				u8 PACK[] = NTAG_PACK;
				ret = writeTag(data, PWD, PACK);
				break;
			}
		}
	}
	
	DnfcStopScanning();
	printf("\n");
	return ret;
}

Result nfc_restore(u8 *data, int datalen, u8 *PWD, int PWDLength) {
	
}

int nfc_init() {
	Result ret = nfcInit(NFC_OpType_RawNFC);
	if(R_FAILED(ret)) {
		printf("nfcInit() failed: 0x%08x.\n", (unsigned int)ret);
		return 0;
	}
	return 1;
}
void nfc_exit() {
	nfcExit();
}
