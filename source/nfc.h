#pragma once

//result size of a block read (READ command)
#define NTAG_PAGE_SIZE 4
#define NTAG_FAST_READ_PAGE_COUNT 15
#define NTAG_READ_PAGE_COUNT 4
#define NTAG_BLOCK_SIZE NTAG_READ_PAGE_COUNT * NTAG_PAGE_SIZE

Result nfc_readFull(u8 *data, int datalen);
Result nfc_readBlock(int pageId, u8 *data, int datalen); //reads four pages
Result nfc_write(u8 *data, int datalen, u8 *PWD, int PWDLength);
