#pragma once

#include <3ds.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_AMIIBO_NAME 512

struct AmiiboIdStruct {
	u16 brand;
	u8 variant;
	u8 type;
	u16 amiiboId;
	u16 series;
};

struct AmiiboIdStruct* parseCharData(const u8* data);
int getNameByAmiiboId(u16 id, char *name, int length);

#ifdef __cplusplus
}
#endif
