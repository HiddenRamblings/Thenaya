#ifndef _AMITOOL_H_
#define _AMITOOL_H_

#define AMIIBO_MAX_SIZE 572

int amitool_setKeys(uint8_t* keydata, int len);
int amitool_setKeysUnfixed(uint8_t* keydata, int len);
int amitool_setKeysFixed(uint8_t* keydata, int len);
int amitool_unpack(uint8_t* tag, int taglen, uint8_t* rdata, int rdatalen);
int amitool_pack(uint8_t* tag, int taglen, uint8_t* rdata, int rdatalen);

#endif