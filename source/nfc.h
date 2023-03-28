#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define NFC_TIMEOUT  200 * 1000000

//result size of a block read (READ command)
#define NTAG_PAGE_SIZE 4
#define NTAG_FAST_READ_PAGE_COUNT 15
#define NTAG_READ_PAGE_COUNT 4
#define NTAG_BLOCK_SIZE NTAG_READ_PAGE_COUNT * NTAG_PAGE_SIZE

Result nfc_readFull(uint8_t *data, int datalen);
Result nfc_readBlock(int pageId, uint8_t *data, int datalen); //reads four pages
Result nfc_write(uint8_t *data, int datalen, uint8_t *PWD, int PWDLength, int fullWrite);
int nfc_init();
void nfc_exit();

#ifdef __cplusplus
}
#endif
