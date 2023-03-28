/*
 * ====================================================================
 * amiibo-generator Copyright (C) 2020 hax0kartik
 * Copyright (C) 2023 AbandonedCart @ Thenaya
 * ====================================================================
 */
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

#include "foomiibo.h"
#include "main.h"
#include "tag.h"
#include "nfc3d/amitool.h"
#include "ui.h"
#include "amiibolookup.h"

uint8_t* getRandomBytes(size_t size) {
	uint8_t *random = (uint8_t*)malloc(size);
	size_t i;
	for (i = 0; i < size; i++) {
		random[i] = rand ();
	}
	return random;
}

uint8_t* generateRandomUID() {
	uint8_t *uid = getRandomBytes(9);
	uid[0x0] = 0x04;
	uid[0x3] = 0x88 ^ uid[0] ^ uid[1] ^ uid[2];
	uid[0x8] = uid[4] ^ uid[5] ^ uid[6] ^ uid[7];
	return uid;
}

uint8_t* generateData(uint16_t amiiboId) {
	uint8_t arr[AMIIBO_MAX_SIZE];

	// Set UID, BCC0
	// 0x04, (byte) 0xC0, 0x0A, 0x46, 0x61, 0x6B, 0x65, 0x0A
	uint8_t *uid = generateRandomUID();
	memcpy(&arr[0x1D4], &uid, sizeof(uid));

	// Set BCC1
	arr[0] = uid[0x8];

	// Set Internal, Static Lock, and CC

	uint8_t cc[7] = { 0x48, 0x0F, 0xE0, 0xF1, 0x10, 0xFF, 0xEE };
	memcpy(&arr[0x1], &cc, 7);

	// Set 0xA5, Write Counter, and Unknown
	uint8_t unknown[4] = { 0xA5, 0x00, 0x00, 0x00 };
	memcpy(&arr[0x28], &unknown, 4);

	// Set Dynamic Lock, and RFUI
	uint8_t rfui[4] = { 0x01, 0x00, 0x0F, 0xBD };
	memcpy(&arr[0x208], &rfui, 4);

	// Set CFG0
	uint8_t cfg0[4] = { 0x00, 0x00, 0x00, 0x04 };
	memcpy(&arr[0x20C], &cfg0, 4);

	// Set CFG1
	uint8_t cfg1[4] = { 0x5F, 0x00, 0x00, 0x00 };
	memcpy(&arr[0x210], &cfg1, 4);

	// Set Keygen Salt
	uint8_t *salt = getRandomBytes(32);
	memcpy(&arr[0x1E8], &salt, 32);
	// Write Identification Block
	memcpy(&arr[0x54], &amiiboId, 16);
	memcpy(&arr[0x1DC], &amiiboId, 16);
	return arr;
}

void printTagName(uint8_t *data) {
	char tagName[MAX_AMIIBO_NAME];
	uint8_t charId[TAG_CHAR_ID_LENGTH];
	int res = tag_charIdDataFromTag(data, sizeof(data), charId, sizeof(charId));
	if (res != TAG_ERR_OK) {
		printf(tagName, sizeof(tagName), "%02X%02X%02X%02X%02X%02X%02X", data[0], data[1], data[2], data[3], data[4], data[5], data[6]); //uid
	} else {
		struct AmiiboIdStruct *charinfo = parseCharData(charId);

		if (!getNameByHexId(charinfo->amiiboId, tagName, sizeof(tagName))) {
			printf(tagName, sizeof(tagName), "%02X%02X%02X%02X%02X%02X%02X", data[0], data[1], data[2], data[3], data[4], data[5], data[6]); //uid
		}
	}
}

u32 showMenuFoomiibo() {
  uiSelectMain();
	uiClearScreen();
	printf("\e[2;1H X - Generate Mario.");
	printf("\e[3;1H A - Coming Soon!");
	printf("\e[2;26H Y - Coming Soon!");
	printf("\e[3;26H B - Quit.");
	uiSelectLog();
	return uiGetKey(KEY_X | KEY_A | KEY_Y | KEY_B);
}

void menuFoomiibo() {
	while (aptMainLoop()) {
		u32 kDown = showMenuFoomiibo();

		if (kDown & KEY_X) {
			printTagName(generateData(0x0000000000000002));
		} else if ((kDown & KEY_A)) {
			break;
		} else if (kDown & KEY_Y) {
			break;
		} else if (kDown & KEY_B) {
      menuExtras();
    }
	}
}
