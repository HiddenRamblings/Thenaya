#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define CMD_READ 0x30
#define CMD_FAST_READ 0x3A
#define CMD_WRITE 0xA2

#define N2_GET_VERSION 0x55
#define N2_ACTIVATE_BANK 0xA7
#define N2_FAST_READ 0x3B
#define N2_FAST_WRITE 0xAE
#define N2_BANK_COUNT 0x55
#define N2_LOCK 0x46
#define N2_READ_SIG 0x43
#define N2_SET_BANKCOUNT 0xA9
#define N2_UNLOCK_1 0x44
#define N2_UNLOCK_2 0x45
#define N2_WRITE 0xA5
#define SECTOR_SELECT 0xC2

uint8_t* elite_getBankCount();
uint8_t* elite_readSignature();
void elite_amiiboLock();
uint8_t* elite_amiiboPrepareUnlock();
void elite_amiiboUnlock();
Result elite_write(uint8_t *data);
void menuElite();

#ifdef __cplusplus
}
#endif
