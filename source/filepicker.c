//by cpasjuste 
//https://gbatemp.net/threads/filepicker-a-very-simple-file-picker-code-for-developers.397917/
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/dirent.h>
 
#define MAX_LINE 10
typedef struct {
	char files[512][512];
	int count;
	int now;
} picker_s;
 
picker_s *picker;
 
bool end_with(const char *str, const char c)
{
	return (str && *str && str[strlen(str) - 1] == c) ? true : false;
}
 
void get_dir(const char *path)
{
	DIR *fd;
	struct dirent *file;
 
	if (NULL == (fd = opendir (path)))
	{
		printf("\xb[3;1HDirectory empty...");
		return;
	}
 
	while ((file = readdir(fd)))
	{
		if (!strcmp (file->d_name, "."))
			continue;
		if (!strcmp (file->d_name, ".."))
			continue;
 
		if(file->d_type != DT_DIR )
		{
				strncpy(picker->files[picker->count], file->d_name, 512);
				picker->count++;
		}
	}
}
 
int pick_file(char *picked, const char *path)
{
	picker = malloc(sizeof(picker_s));
	memset(picker, 0, sizeof(picker_s));

	get_dir(path);
	consoleClear();
	
	int result = 0;

	while (aptMainLoop())
	{
		hidScanInput();
		u32 kDown = hidKeysDown();
		if (kDown & KEY_B) break;

		if(kDown & KEY_DOWN)
		{
				picker->now++;
				if(picker->now >= picker->count)
						picker->now = 0;
				consoleClear();
		}

		if(kDown & KEY_UP)
		{
				picker->now--;
				if(picker->now < 0)
						picker->now = picker->count-1;
				consoleClear();
		}

		if(kDown & KEY_A)
		{
				if(end_with(path, '/'))
						snprintf(picked, 512, "%s%s", path, picker->files[picker->now]);
				else
						snprintf(picked, 512, "%s/%s", path, picker->files[picker->now]);
				result = 1;
				break;
		}

		int i;
		for(i=0; i<MAX_LINE; i++)
		{
				int index = i+picker->now;
				if(index >= picker->count) break;

				if(i==0)
						printf("\x1b[%i;1H\x1b[31m%s\x1b[0m", i+1, picker->files[index]);
				else
						printf("\x1b[%i;1H%s\n", i+1, picker->files[index]);
		}

		gfxFlushBuffers();
		gfxSwapBuffers();
		gspWaitForVBlank();
	}
   
	free(picker);
	consoleClear();
	return result;
}