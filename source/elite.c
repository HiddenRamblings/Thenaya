#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

#include <3ds.h>
#include <3ds/errf.h>
#include <3ds/services/nfc.h>
#include <3ds/services/apt.h>

#include "elite.h"
#include "main.h"
#include "nfc.h"
#include "nfc3d/amitool.h"
#include "ui.h"

#define DnfcStartOtherTagScanning nfcStartOtherTagScanning
#define DnfcGetTagState nfcGetTagState
#define DnfcSendTagCommand nfcSendTagCommand
#define DnfcSendTagCommand nfcSendTagCommand
#define DnfcStopScanning nfcStopScanning

uint8_t* elite_getBankCount() {
  uiUpdateStatus("Getting bank count..");
  uint8_t req[1];
  req[0] = N2_BANK_COUNT;
  return (uint8_t*) elite_write(req);
}

uint8_t* elite_readSignature() {
  uiUpdateStatus("Reading signature..");
  uint8_t req[1];
  req[0] = N2_READ_SIG;
  return (uint8_t*) elite_write(req);
}

void elite_setBankCount(int count) {
  uiUpdateStatus("Setting bank count..");
  uint8_t req[2];
  req[0] = N2_SET_BANKCOUNT;
  req[1] = count;
  elite_write(req);
}

void elite_activateBank(int bank) {
  uiUpdateStatus("Activating bank..");
  uint8_t req[2];
  req[0] = N2_ACTIVATE_BANK;
  req[1] = bank;
  elite_write(req);
}

void elite_initFirmware() {
  uint8_t req[16];
  req[0] = (uint8_t) 0xFFF4;
  req[1] = 0x49;
  req[2] = (uint8_t) 0xFF9B;
  req[3] = (uint8_t) 0xFF99;
  req[4] = (uint8_t) 0xFFC3;
  req[5] = (uint8_t) 0xFFDA;
  req[6] = 0x57;
  req[7] = 0x71;
  req[8] = 0x0A;
  req[9] = 0x64;
  req[10] = 0x4A;
  req[11] = (uint8_t) 0xFF9E;
  req[12] = (uint8_t) 0xFFF8;
  req[13] = CMD_WRITE;
  req[14] = CMD_READ;
  req[15] = (uint8_t) 0xFFD9;
  elite_write(req);
}

uint8_t* elite_fastRead(int startAddr, int endAddr) {
  uint8_t req[3] = {CMD_FAST_READ, startAddr, endAddr};
  return (uint8_t*) elite_write(req);
}

uint8_t* elite_amiiboFastRead(int startAddr, int endAddr, int bank) {
  uint8_t req[4] = {N2_FAST_READ, startAddr, endAddr, bank};
  return (uint8_t*) elite_write(req);
}

bool elite_amiiboWrite(int addr, int bank, uint8_t *data) {
  uint8_t req[7] = {N2_WRITE, addr, bank, data[0], data[1], data[2], data[3]};
  int ret = elite_write(req);
  return R_FAILED(ret);
}

bool elite_amiiboFastWrite(int addr, int bank, uint8_t *data) {
  uint8_t req[sizeof(data)+3];
  req[0] = N2_FAST_WRITE;
  req[1] = addr;
  req[2] = bank;
  for(int x = 0; x<sizeof(data); x++) {
		req[3+x] = data[x];
	}
  int ret = elite_write(req);
  return R_FAILED(ret);
}

void elite_amiiboLock() {
  uint8_t req[1];
  req[0] = N2_LOCK;
  elite_write(req);
}

uint8_t* elite_amiiboPrepareUnlock() {
  uint8_t req[1];
  req[0] = N2_UNLOCK_1;
  return (uint8_t*) elite_write(req);
}

void elite_amiiboUnlock() {
  uint8_t req[1];
  req[0] = N2_UNLOCK_2;
  elite_write(req);
}

Result elite_write(uint8_t *data) {
	Result ret = 0;
  NFC_TagState prevstate, curstate;

  uiSelectMain();
	//todo: show title as write to tag / restore tag
	printf("\e[2J\e[H\e[0m\e[5;2HPlace tag on scanner, or press B to cancel");
	uiUpdateStatus("Waiting...");

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
        uint8_t buffer[AMIIBO_MAX_SIZE];
				memset(buffer, 0, sizeof(buffer));
        size_t resultsize = sizeof(buffer);

      	int ret = DnfcSendTagCommand(data, sizeof(data), buffer, sizeof(buffer), &resultsize, NFC_TIMEOUT);

      	if(R_FAILED(ret)) {
      		printf("Writing to N2 failed: 0x%08x.\n", (unsigned int)ret);
      	} else if (resultsize >=1 && buffer[0] != 0x0A) {
      		printf("write to N2 returned a NAK %d\n", buffer[0]);
        }
      	if(R_FAILED(nfcCmd22())) {  //power down the tag
      		printf("nfcCmd22 failed: 0x%08x.\n", (unsigned int)ret);
      	}
      	break;
			}
		}
	}

	DnfcStopScanning();
	printf("\n");
	if(R_FAILED(ret)) {
		goto writeToTag_ERROR;
	}

	uiUpdateStatus("Complete.");
	uiUpdateProgress(0, -1);
	uiSelectMain();
	printf("\e[2J\e[H\e[0m\e[5;2HFinished writing to N2.\n\n   Press A to continue.");
	uiGetKey(KEY_A);
	return ret;

writeToTag_ERROR:
	uiUpdateStatus("ERROR");
	uiSelectMain();
	printf("\e[2J\e[H\e[0m\e[5;2HWrite to N2 failed.\n\n   Press A to continue.");
	uiGetKey(KEY_A);
  return ret;
}

void showBankCount() {
  uint8_t *banks = elite_getBankCount();
	printf("%u banks active\n", banks[1]);
}

u32 showMenuElite() {
  uiSelectMain();
	uiClearScreen();
	printf("\e[2;1H X - Get bank count.");
	printf("\e[3;1H A - Coming Soon!");
	printf("\e[2;26H Y - Coming Soon!");
	printf("\e[3;26H B - Quit.");
	uiSelectLog();
	return uiGetKey(KEY_X | KEY_A | KEY_Y | KEY_B);
}

void menuElite() {
	while (aptMainLoop()) {
		u32 kDown = showMenuElite();

		if (kDown & KEY_X) {
			showBankCount();
		} else if ((kDown & KEY_A)) {
			break;
		} else if (kDown & KEY_Y) {
			break;
		} else if (kDown & KEY_B) {
      menuExtras();
    }
	}
}
