#pragma once

#include <3ds.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_AMIIBO_NAME 512

struct AmiiboIdStruct {
	uint16_t brand;
	uint8_t variant;
	uint8_t type;
	uint16_t amiiboId;
	uint16_t series;
};

struct AmiiboIdStruct* parseCharData(const uint8_t* data);
int getNameByAmiiboId(uint16_t id, char *name, int length);
int getNameByHexId(uint16_t id, char *name, int length);

#ifdef __cplusplus
}
#endif
