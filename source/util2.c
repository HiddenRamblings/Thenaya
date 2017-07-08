#include "util2.h"

#include <stdio.h>

/*
void printbuf(char *prefix, u8* data, size_t len) {
	char bufstr[len*2 + 2];
	memset(bufstr, 0, sizeof(bufstr));
	for(int pos=0; pos<len; pos++)snprintf(&bufstr[pos*2], 3, "%02x", data[pos]);
	printf("%s hex: %s\n", prefix, bufstr);
}*/

void printbuf(char *prefix, u8* data, size_t len) {
	char bufstr[len*3 + 3];
	memset(bufstr, 0, sizeof(bufstr));
	for(int pos=0; pos<len; pos++) {
		snprintf(&bufstr[pos*3], 4, "%02x ", data[pos]);
		if (pos > 0 && pos % 12 == 0) {
			bufstr[pos*3+2] = '\n';
		}
	}
	printf("%s hex: %s\n", prefix, bufstr);
}

int readFile(char *filepath, u8 *data, u32 datasize) {
	struct stat filestats;
	FILE *f;
	int readsize=0;

	if(stat(filepath, &filestats)==-1) return -1;

	if (filestats.st_size > datasize) {
		printf("File too large: 0x%08x.\n", (unsigned int)filestats.st_size);
		return -2;
	}
	
	f = fopen(filepath, "r");
	if(f==NULL) return -3;

	readsize = fread(data, 1, filestats.st_size, f);
	fclose(f);

	if(readsize!=filestats.st_size) return -4;

	return readsize;
}

int writeFile(char *filepath, u8 *data, u32 datasize) {
	FILE *f;
	int writesize=0;

	f = fopen(filepath, "w");
	if(f==NULL) return -3;

	writesize = fwrite(data, 1, datasize, f);
	fclose(f);

	if(writesize!=datasize) return -4;
	return writesize;
}

void cleanFilename(char *filename) {
	int len = strlen(filename);
	for (int i=0; i<len; i++) {
		switch (filename[i]) {
			case '/':
			case '\\':
				filename[i] = '_';
		}
	}
}