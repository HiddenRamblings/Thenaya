#include <string.h>

#include "util.h"
#include "mbedtls/sha256.h"
#include "nfc3d/amiibo.h"
#include "nfc3d/keygen.h"
#include "nfc3d/amitool.h"

static nfc3d_amiibo_keys keys;

char CHECKSUM_UNFIXED[]= { 0x86,0x81,0x06,0x13,0x59,0x41,0xCB,0xCA,0xB3,0x55,0x2B,0xD1,0x48,0x80,0xA7,0xA3,0x43,0x04,0xEF,0x34,0x09,0x58,0xA6,0x99,0x8B,0x61,0xA3,0x8B,0xA3,0xCE,0x13,0xD3 };

char CHECKSUM_LOCKED[] = { 0xB4,0x87,0x27,0x79,0x7C,0xD2,0x54,0x82,0x00,0xB9,0x9C,0x66,0x5B,0x20,0xA7,0x81,0x90,0x47,0x01,0x63,0xCC,0xB8,0xE5,0x68,0x21,0x49,0xF1,0xB2,0xF7,0xA0,0x06,0xCF };

int amitool_setKeys(uint8_t* keydata, int len) {
	if (sizeof(keys) != len)
		return -1;

	//we allow for the two keys to be in any order in the file

	int res = amitool_setKeysUnfixed(keydata, 80);
	if (res == 0) {
		res = amitool_setKeysFixed(&keydata[80], 80);
		if (res != 0)
			return -2;
	} else {
		res = amitool_setKeysUnfixed(&keydata[80], 80);
		if (res != 0)
			return -2;
		int res = amitool_setKeysFixed(keydata, 80);
		if (res != 0)
			return -2;
	}
	return 0;
}

int amitool_setKeysUnfixed(uint8_t* keydata, int len) {
	if (sizeof(keys.data) != len)
		return -2;

	uint8_t checksum[32];
	mbedtls_sha256(keydata, len, checksum, 0);

	if (memcmp(CHECKSUM_UNFIXED, checksum, sizeof(checksum)))
		return -2;


	memcpy(&keys.data, keydata, len);
	return 0;
}

int amitool_setKeysFixed(uint8_t* keydata, int len) {
	if (sizeof(keys.tag) != len)
		return -1;

	uint8_t checksum[32];
	mbedtls_sha256(keydata, len, checksum, 0);

	if (memcmp(CHECKSUM_LOCKED, checksum, sizeof(checksum)))
		return -2;

	memcpy(&keys.tag, keydata, len);
	return 0;
}

int amitool_unpack(uint8_t* tag, int taglen, uint8_t* rdata, int rdatalen) {
	if (taglen< NFC3D_AMIIBO_SIZE || rdatalen< NFC3D_AMIIBO_SIZE || rdatalen < taglen )
		return 0;

	memcpy(rdata, tag, taglen); //copy any extra data in source to destination
	if (!nfc3d_amiibo_unpack(&keys, tag, rdata))
		return 0;

	return 1;
}

int amitool_pack(uint8_t* tag, int taglen, uint8_t* rdata, int rdatalen) {
	if (taglen< NFC3D_AMIIBO_SIZE || rdatalen< NFC3D_AMIIBO_SIZE || rdatalen < taglen)
		return 0;

	memcpy(rdata, tag, taglen); //copy any extra data in source to destination
	nfc3d_amiibo_pack(&keys, tag, rdata);

	return 1;
}