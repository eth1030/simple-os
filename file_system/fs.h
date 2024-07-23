#ifndef INCLUDE_FS_H
#define INCLUDE_FS_H

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_FILENAME_SIZE 15
#define MAX_FD_COUNT 32
#define MAX_FILE_COUNT 64
#define MAX_FILE_SIZE 1048576

/*
        Block
        0 superblock
        1 dir
        2 inode bitmap
        3 inodes
        4 data bitmap
        5 data...

*/

/*
        Global vars
        s superblck
        used_block_bitmap
        used_inode_bitmap
        i[MAX_FILE_COUNT] inode table
        d[MAX_FILE_CONUT] directory entries
        fdt[MAX_FD_COUNT] file descriptor table
*/

/* define data structures*/
typedef struct super_block {
    uint16_t used_block_bitmap_count;
    uint16_t used_block_bitmap_offset;
    uint16_t inode_metadata_blocks;
    uint16_t inode_metadata_offset;
    uint16_t used_inode_bitmap_offset;
    uint16_t used_inode_bitmap_count;
    uint16_t directory_offset;
    uint16_t directory_blocks;
    uint16_t data_offset;
} super_block;

typedef struct inode {
    uint16_t direct_offset;
    uint16_t single_indirect_offset;
    uint16_t double_indirect_offset;
    uint16_t file_size;
} inode;

typedef struct dir_entry {
    bool is_used;
    uint16_t inode_number;
    char name[16];
    uint16_t refcount;
} dir_entry;

typedef struct file_descriptor {
    bool is_used;
    uint16_t inode_number;
    uint16_t offset;
}file_descriptor;

int make_fs(const char *disk_name);
int mount_fs(const char *disk_name);
int umount_fs(const char *disk_name);
int fs_open(const char *name);
int fs_close(int fildes);
int fs_create(const char *name);
int fs_delete(const char *name);
int fs_read(int fildes, void *buf, size_t nbyte);
int fs_write(int fildes, void *buf, size_t nbyte);
int fs_get_filesize(int fildes);
int fs_listfiles(char ***files);
int fs_lseek(int fildes, off_t offset);
int fs_truncate(int fildes, off_t length);
#endif /* INCLUDE_FS_H */
