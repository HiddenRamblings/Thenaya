#include "tag.h"

#include "nfc3d/amitool.h"
#include <stdio.h>
#include <string.h>

static u8 unpackedData[AMIIBO_MAX_SIZE]; //holds the data in internal (unpacked format)
static int dataLength;
static int amiiboLoaded = 0;
static int keysLoaded = 0;

int tag_setKeys(u8 *keybuffer, int size) {
	if (keysLoaded) return TAG_ERR_OK;
	if (amitool_setKeys(keybuffer, size) != 0) {
		return TAG_ERR_INVALID_KEY;
	}
	keysLoaded = 1;
	return TAG_ERR_OK;
}

int tag_isLoaded() {
	return amiiboLoaded;
}

int tag_isKeysLoaded() {
	return keysLoaded;
}

/*
loads amiibo in tag format, but holds it in internal format
*/
int tag_setTag(u8 *data, int size) {
	amiiboLoaded = 0;
	memset(unpackedData, 0, AMIIBO_MAX_SIZE);
	memcpy(unpackedData, data, size);

	if (size > AMIIBO_MAX_SIZE)
		return TAG_ERR_INVALID_SIZE;
	
	if (!tag_isValid(data, size))
		return TAG_ERR_VALIDATION_FAILED;
	
	if (!keysLoaded)
		return TAG_KEY_NOT_LOADED;
	
	int res = amitool_unpack(data, size, unpackedData, AMIIBO_MAX_SIZE);
	if (!res) {
		return TAG_ERR_DECRYPT_FAIL;
	}
	
	amiiboLoaded = 1;
	dataLength = size;
	return TAG_ERR_OK;
}

#define PAGED_BYTE(page, index) ((page *4) + index)

/*
Validates if the data is an amiibo (expects tag format data)
*/
int tag_isValid(u8 *data, int size) {
	// must start with a 0x04.
	if (data[0x0] != 0x04)
		return 0;

	//lock signature mismatch
	if (data[PAGED_BYTE(0x02, 2)] != 0x0F || data[PAGED_BYTE(0x02, 3)] != 0xE0)
		return 0;
	
	// CC signature mismatch.
	if (data[PAGED_BYTE(0x03, 0)] != 0xF1 || data[PAGED_BYTE(0x03, 1)] != 0x10
	 || data[PAGED_BYTE(0x03, 2)] != 0xFF || data[PAGED_BYTE(0x03, 3)] != 0xEE)
		return 0;

	//dynamic lock signature mismatch.
	if (data[PAGED_BYTE(0x82, 0)] != 0x01 || data[PAGED_BYTE(0x82, 1)] != 0x0
	 || data[PAGED_BYTE(0x82, 2)] != 0x0F)
		return 0;

	//CFG0 signature mismatch
	if (data[PAGED_BYTE(0x83, 0)] != 0x00 || data[PAGED_BYTE(0x83, 1)] != 0x00
	 || data[PAGED_BYTE(0x83, 2)] != 0x00 || data[PAGED_BYTE(0x83, 3)] != 0x04)
		return 0;

	//CFG1 signature mismatch
	if (data[PAGED_BYTE(0x84, 0)] != 0x5F || data[PAGED_BYTE(0x84, 1)] != 0x00
	 || data[PAGED_BYTE(0x84, 2)] != 0x00 || data[PAGED_BYTE(0x84, 3)] != 0x00)
		return 0;

	return 1;
}

/*
extracts 7 byte uid from 9 byte uid
*/
int tag_getUidFromBlock(u8 *data, int size, u8 *uid, int uidsize) {
	//uid without checksums (7 bytes), usefull for calculating write password
	if (size < 8 || uidsize < 7)
		return TAG_ERR_BUFFER_TOO_SMALL;
	uid[0] = data[0];
	uid[1] = data[1];
	uid[2] = data[2];
	uid[3] = data[4];
	uid[4] = data[5];
	uid[5] = data[6];
	uid[6] = data[7];
	return TAG_ERR_OK;
}

int tag_setUid(u8* uid, int uidlen) {
	//we handle both 7 byte uid and 9 byte uid (7+2 checksums)
	if (!amiiboLoaded)
		return TAG_ERR_NO_TAG_LOADED;
	if (uidlen == 7) {
		u8 uid9[9];
		uid9[0] = uid[0];
		uid9[1] = uid[1];
		uid9[2] = uid[2];
		uid9[3] = 0x88 ^ uid[0] ^ uid[1] ^ uid[2];
		uid9[4] = uid[3];
		uid9[5] = uid[4];
		uid9[6] = uid[5];
		uid9[7] = uid[6];
		uid9[8] = uid[3] ^ uid[4] ^ uid[5] ^ uid[6];
		memcpy(&unpackedData[0x1d4], uid9, 8);
		unpackedData[0] = uid9[8];
	} else if (uidlen == 9) {
		memcpy(&unpackedData[0x1d4], uid, 8);
		unpackedData[0] = uid[8];
	} else
		return TAG_ERR_INVALID_BUFFER_SIZE;
	
	return TAG_ERR_OK;
}

/*
returns amiibo data in tag format
*/
int tag_getTag(u8 *data, int size) {
	if (!amiiboLoaded)
		return TAG_ERR_NO_TAG_LOADED;
	if (size < dataLength)
		return TAG_ERR_BUFFER_TOO_SMALL;
	if (size > dataLength)
		memset(data, 0, size);
	
	if (!keysLoaded)
		return TAG_KEY_NOT_LOADED;
	
	int res = amitool_pack(unpackedData, dataLength, data, size);
	if (!res) {
		return TAG_ERR_ENRYPT_FAIL;
	}
	return TAG_ERR_OK;
}

int tag_calculatePassword(u8 *uid, int uidlen, u8 *pwd, int pwdlen) {
	if (uidlen!=7)
		return TAG_ERR_INVALID_BUFFER_SIZE;
	if (pwdlen < TAG_PWD_LEN)
		return TAG_ERR_BUFFER_TOO_SMALL;
	
	pwd[0] = 0xAA ^ (uid[1] ^ uid[3]);
	pwd[1] = 0x55 ^ (uid[2] ^ uid[4]);
	pwd[2] = 0xAA ^ (uid[3] ^ uid[5]);
	pwd[3] = 0x55 ^ (uid[4] ^ uid[6]);
	
	return TAG_ERR_OK;
}

/*
Returns character id bytes from tag format
*/
int tag_charIdDataFromTag(u8 *data, int dataLen, u8 *charData, int charDataLen) {
	if (dataLen < 0x5C)
		return TAG_ERR_INVALID_SIZE;
	if (charDataLen < TAG_CHAR_ID_LENGTH)
		return TAG_ERR_BUFFER_TOO_SMALL;
	memcpy(charData, &data[0x54], TAG_CHAR_ID_LENGTH);
	return TAG_ERR_OK;
}


/*
returns the char id bytes from loaded tag
*/
int tag_getCharIdData(u8 *charData, int charDataLen) {
	if (!amiiboLoaded)
		return TAG_ERR_NO_TAG_LOADED;
	if (charDataLen < TAG_CHAR_ID_LENGTH)
		return TAG_ERR_BUFFER_TOO_SMALL;
	memcpy(charData, &unpackedData[0x1DC], TAG_CHAR_ID_LENGTH);
	return TAG_ERR_OK;
}

/*
returns 7 byte uid from loaded tag
*/
int tag_getUid7(u8 *uid, int uidlen) {
	if (!amiiboLoaded)
		return TAG_ERR_NO_TAG_LOADED;
	if (uidlen < 7)
		return TAG_ERR_BUFFER_TOO_SMALL;
	uid[0] = unpackedData[0x1D4];
	uid[1] = unpackedData[0x1D5];
	uid[2] = unpackedData[0x1D6];
	uid[3] = unpackedData[0x1D8];
	uid[4] = unpackedData[0x1D9];
	uid[5] = unpackedData[0x1DA];
	uid[6] = unpackedData[0x1DB];
	return TAG_ERR_OK;
}