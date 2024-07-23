#include "../fs.h"
#include "../disk.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>


int main() {
    char buffer[MAX_FILE_SIZE];
    char s = 's';
    char l = '\0';
    memcpy(buffer, &s, MAX_FILE_SIZE - 1);
    memcpy(buffer + MAX_FILE_SIZE - 1, &l, 1);
    char buf[1024];
    // create fs
    const char *name = "fs1";
    char ***files;
    if(make_fs(name) == -1)
        return -1;
    if(mount_fs(name) == -1)
        return -1;
    if(fs_create("hi") == -1)
        return -1;
    if(fs_open("hi") == -1)
        return -1;
    if (fs_write(0,buffer,MAX_FILE_SIZE) == -1)
        return -1;
    if(fs_create("hi1") == -1)
        return -1;
    if(fs_open("hi1") == -1)
        return -1;
    if (fs_write(1,buffer,MAX_FILE_SIZE) == -1)
        return -1;
    // if(fs_close(1) == -1)
    //     return -1;
    if(fs_read(1, buf,MAX_FILE_SIZE) == -1)
        return -1;    
    // if(fs_get_filesize(5) == 0)
    //     return -1;
    // if(fs_get_filesize(0) == -1)
    //     return -1;
    // if (fs_write(0,buffer,5) == -1)
    //     return -1;
    // if (fs_listfiles(files) == -1)
    //     return -1;
    // if (fs_truncate(0, 1) == -1)
    //     return -1;
    if(fs_close(1) == -1)
        return -1;
    if (fs_delete("hi1") == -1) 
        return -1;
    if (fs_read(1,buffer,2) != -1)
        return -1;
    if(umount_fs(name) == -1)
        return -1;
    return 0;
}