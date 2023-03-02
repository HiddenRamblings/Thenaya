#include <stdio.h>

#include <3ds.h>
#include <3ds/errf.h>
#include <3ds/services/nfc.h>
#include <3ds/services/apt.h>

#include "elite.h"
#include "nfc.h"
#include "nfc3d/amitool.h"
#include "ui.h"

#define DnfcStartOtherTagScanning nfcStartOtherTagScanning
#define DnfcGetTagState nfcGetTagState
#define DnfcSendTagCommand nfcSendTagCommand
#define DnfcSendTagCommand nfcSendTagCommand
#define DnfcStopScanning nfcStopScanning

u32 showEliteMenu() {
	return uiGetKey(KEY_X | KEY_A | KEY_Y | KEY_B);
}

u8* elite_getBankCount() {
  u8 req[1];
  req[0] = N2_BANK_COUNT;
  return (u8*) elite_write(req);
}

u8* elite_readSignature() {
  u8 req[1];
  req[0] = N2_READ_SIG;
  return (u8*) elite_write(req);
}

void elite_setBankCount(int count) {
  u8 req[2];
  req[0] = N2_SET_BANKCOUNT;
  req[1] = count;
  elite_write(req);
}

void elite_activateBank(int bank) {
  u8 req[2];
  req[0] = N2_ACTIVATE_BANK;
  req[1] = bank;
  elite_write(req);
}

void elite_initFirmware() {
  u8 req[16];
  req[0] = (u8) 0xFFF4;
  req[1] = 0x49;
  req[2] = (u8) 0xFF9B;
  req[3] = (u8) 0xFF99;
  req[4] = (u8) 0xFFC3;
  req[5] = (u8) 0xFFDA;
  req[6] = 0x57;
  req[7] = 0x71;
  req[8] = 0x0A;
  req[9] = 0x64;
  req[10] = 0x4A;
  req[11] = (u8) 0xFF9E;
  req[12] = (u8) 0xFFF8;
  req[13] = CMD_WRITE;
  req[14] = CMD_READ;
  req[15] = (u8) 0xFFD9;
  elite_write(req);
}

u8* elite_fastRead(int startAddr, int endAddr) {
  u8 req[3] = {CMD_FAST_READ, startAddr, endAddr};
  return (u8*) elite_write(req);
}

u8* elite_amiiboFastRead(int startAddr, int endAddr, int bank) {
  u8 req[4] = {N2_FAST_READ, startAddr, endAddr, bank};
  return (u8*) elite_write(req);
}

bool elite_amiiboWrite(int addr, int bank, u8 *data) {
  u8 req[7] = {N2_WRITE, addr, bank, data[0], data[1], data[2], data[3]};
  int ret = elite_write(req);
  return R_FAILED(ret);
}

bool elite_amiiboFastWrite(int addr, int bank, u8 *data) {
  u8 req[sizeof(data)+3];
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
  u8 req[1];
  req[0] = N2_LOCK;
  elite_write(req);
}

u8* elite_amiiboPrepareUnlock() {
  u8 req[1];
  req[0] = N2_UNLOCK_1;
  return (u8*) elite_write(req);
}

void elite_amiiboUnlock() {
  u8 req[1];
  req[0] = N2_UNLOCK_2;
  elite_write(req);
}

Result elite_write(u8 *data) {
	Result ret = 0;

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
        u8 buffer[AMIIBO_MAX_SIZE];
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
	return ret;
}
