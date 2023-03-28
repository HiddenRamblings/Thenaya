#pragma once

#include <3ds.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TAG_ERR_OK 0
#define TAG_ERR_INVALID_KEY -1
#define TAG_ERR_DECRYPT_FAIL -2
#define TAG_ERR_INVALID_SIZE -3
#define TAG_ERR_NO_TAG_LOADED -4
#define TAG_ERR_BUFFER_TOO_SMALL -5
#define TAG_ERR_INVALID_BUFFER_SIZE -6
#define TAG_ERR_ENCRYPT_FAIL -7
#define TAG_ERR_VALIDATION_FAILED -8
#define TAG_KEY_NOT_LOADED -9

#define TAG_PWD_LEN 4

//we use the full uid including the checksum bytes
#define TAG_UID9_LENGTH 9
#define TAG_UID7_LENGTH 7
#define TAG_CHAR_ID_LENGTH 8


int tag_setKeys(uint8_t *keybuffer, int size);
int tag_isLoaded();
int tag_isKeysLoaded();
int tag_setTag(uint8_t *data, int datalength);
int tag_isValid(uint8_t *data, int size);
int tag_isLocked(uint8_t *data, int size);
int tag_setUid(uint8_t* uid, int uidlen);
int tag_getTag(uint8_t *data, int size);
int tag_getUidFromBlock(uint8_t *data, int size, uint8_t *uid, int uidsize);
int tag_calculatePassword(uint8_t *uuid, int uuidlen, uint8_t *pwd, int pwdlen);
int tag_charIdDataFromTag(uint8_t *data, int dataLen, uint8_t *chardata, int chardatalen);
int tag_getUid7(uint8_t *uid, int uidlen);
int tag_getCharIdData(uint8_t *charData, int charDataLen);

#ifdef __cplusplus
}
#endif
