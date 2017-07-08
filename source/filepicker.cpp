#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/dirent.h>

#include "ui.h"
#include "filepicker.h"
#include <list>
#include <iostream>

using namespace std;
 
class FileInfo {
	public:
	char* name;
	bool isDir;
	
	FileInfo(char *fileName, bool fileIsDir) {
		int nlen = strlen(fileName);
		name = new char[nlen+1];
		strcpy(name, fileName);
		isDir = fileIsDir;
	}
	
	~FileInfo() {
		delete [] name;
	}
};

static bool compareFileInfo(const FileInfo *a, const FileInfo *b) {
	if (a->isDir && !b->isDir)
		return true;
	if (b->isDir && !a->isDir)
		return false;
	return (strcasecmp(a->name, b->name) < 0);
}

static void getParentDir(const char *dir, char *parent) { //parent must be atleast the same length as dir
	strcpy(parent, dir);
	int i = strlen(parent)-1;
	if (parent[i] == '/')
		parent[i] = '\0';
	i--;
	while (i>0 && parent[i] != '/') {
		parent[i] = '\0';
		i--;
	}
	if (i == 0)
		strcpy(parent, dir);
}

static char *appendPath(const char *root, const char *child) { //caller is responsible for deleting result
	int rootLen = strlen(root);
	char *name = new char[rootLen + strlen(child)+2];
	strcpy(name, root);
	if (root[rootLen-1] != '/') {
		name[rootLen] = '/';
		name[rootLen+1] = '\0';
	}
	strcat(name, child);
	return name;
}

class FileSystem {
  public:
	std::list<FileInfo*> files;
	char *currentDir;

	FileSystem() {
		currentDir = NULL;
	}
	
    ~FileSystem() {
    	clear();
    }
	void clear() {
    	for (auto f : files) {
    		delete f;
    	}
		files.clear();
		if (currentDir != NULL)
			delete [] currentDir;
		currentDir = NULL;
	}
	
	bool load(const char *path) {
		DIR *fd;
		if (NULL == (fd = opendir(path))) {
			return false;
		}

		clear();
		currentDir = new char[strlen(path)+1];
		strcpy(currentDir, path);
	 
		struct dirent *file;
		while ((file = readdir(fd))) {
			if (!strcmp (file->d_name, "."))
				continue;
			if (!strcmp (file->d_name, ".."))
				continue;
	 
			FileInfo *fileitem = new FileInfo(file->d_name, file->d_type == DT_DIR);
			files.push_back(fileitem);
		}
		
		closedir(fd);

		files.sort(compareFileInfo);
		return true;
	}
};

class FilePicker {
	public:
	FileSystem fs;
	int maxLines;
	char *selectedFile;
	
	FilePicker(int maxLines) {
		this->maxLines = maxLines;
		selectedFile = NULL;
	}
	
	~FilePicker() {
		if (selectedFile != NULL) {
			delete [] selectedFile;
		}
	}
	
	void setPath(const char *path) {
		fs.load(path);
	}
	
	void renderList(list<FileInfo*>::iterator start, list<FileInfo*>::iterator end, list<FileInfo*>::iterator selected) {
		printf("\e[0;0H\e[0;7m  A - Select   B - Back               Y - Cancel  \e[0m");
		printf("\e[0m %-47.47s\n", fs.currentDir);
		
		if (*start == NULL) {
			printf("   \e[1;31m[%-44.44s]", "EMPTY DIR");
			for(int i=0; i<maxLines-2; i++) {
				printf("%-50.50s", "");
			}
			return;
		}
		auto f = start;
		for(int i=0; i<maxLines-1; i++) {
			if (f == end) {
				printf("%-50.50s", "");
				continue;
			}
			if (f == selected) {
				if ((*f)->isDir)
					printf("\e[33;1m=> [%-44.44s]\n", (*f)->name);
				else
					printf("\e[36;1m=> %-46.46s\n", (*f)->name);
			} else {
				if ((*f)->isDir)
					printf("   \e[0;33m[%-44.44s]\n", (*f)->name);
				else
					printf("   \e[0;36m%-46.46s\n", (*f)->name);
			}
			f = std::next(f, 1);
		}
	}
	
	bool show() {
		uiSelectMain();
		list<FileInfo*>::iterator begin;
		list<FileInfo*>::iterator end;
		list<FileInfo*>::iterator current;
		list<FileInfo*>::iterator selected;
		int selectedIndex;
		
		bool dirChanged = true;
		
		while (aptMainLoop()) {
			if (dirChanged) {
				begin = fs.files.begin();
				end = fs.files.end();
				current = begin;
				selected = current;
				selectedIndex = 0;
				dirChanged = false;
			}
			renderList(current, end, selected);
			u32 key = uiGetKey(KEY_A | KEY_B | KEY_Y | KEY_UP | KEY_DOWN | KEY_RIGHT | KEY_LEFT);
			if (key & KEY_DOWN) {
				auto selectedNext = std::next(selected, 1);
				if (selectedNext != end) {
					selected = selectedNext;
					selectedIndex++;
					
					if (selectedIndex > (maxLines / 2)) {
						auto next = std::next(current, 1);
						if (next != end)
							current = next;
					}
				}
				continue;
			} else if (key & KEY_UP) {
				if (selected != begin) {
					selected = std::next(selected, -1);
					selectedIndex--;
					
					if (current != begin)
						current = std::next(current, -1);
				}
				continue;
			} else if (key & KEY_A) {
				FileInfo *f = (*selected);
				if (f != NULL) {
					if (f->isDir) {
						char *name = appendPath(fs.currentDir, f->name);
						setPath(name);
						delete [] name;
						dirChanged = true;
					} else {
						char *name = appendPath(fs.currentDir, f->name);
						selectedFile = name; //name buffer will be deleted by the destructor of this class
						return true;
					}
				}
			} else if (key & KEY_RIGHT) {
				FileInfo *f = (*selected);
				if (f != NULL) {
					if (f->isDir) {
						char *name = appendPath(fs.currentDir, f->name);
						setPath(name);
						delete [] name;
						dirChanged = true;
					}
				}
			} else if (key & KEY_B || key & KEY_LEFT) {
				int size = strlen(fs.currentDir);
				char *parent = new char[size+1];
				getParentDir(fs.currentDir, parent);
				
				setPath(parent);
				delete [] parent;
				
				dirChanged = true;
			} else if (key & KEY_Y) {
				printf("Returning\n");
				return false;
			}
		}
		return false;
	}
};

int fpPickFile(const char *path, char *selectedFile, unsigned int filenameSize) {
	FilePicker fp(24);
	fp.setPath(path);
	if (!fp.show())
		return 0;
	if (filenameSize < (strlen(fp.selectedFile)+1))
		return 0;
	strcpy(selectedFile, fp.selectedFile);
	return 1;
}