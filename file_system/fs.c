#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "disk.h"
#include "fs.h"

/* global variables*/
uint8_t used_block_bitmap[DISK_BLOCKS];
uint8_t used_inode_bitmap[MAX_FILE_COUNT];
super_block *s;
// super_block temp_s;
inode i[MAX_FILE_COUNT];
dir_entry d[MAX_FILE_COUNT];
bool mounted = false;
file_descriptor fdt[MAX_FD_COUNT];

int make_fs(const char *disk_name) {
    /*create a fresh (and empty) file system on the virtual disk with name disk_name
    open this disk and write/initialize the necessary meta-information for your file system so
    that it can be later used (mounted). The function returns 0 on success, and -1 if the disk 
    disk_name could not be created, opened, or properly initialized.*/ 

    // invoke make_disk(disk_name) to create a new disk
    if (make_disk(disk_name) == -1) {
        return -1;
    }
    
    // open disk to write
    if (open_disk(disk_name) == -1) {
        return -1;
    }

    // create buffer
    char buffer[BLOCK_SIZE];

    // create fd table
    for (int it = 0; it < MAX_FD_COUNT; it++) {
        fdt[it].is_used = false;
        fdt[it].inode_number = -1;
        fdt[it].offset = -1;
    }

    // ***************** 0: initialize superblock *****************
    super_block temp_s;
    temp_s.used_block_bitmap_count = (sizeof(char) * DISK_BLOCKS); // space block bm takes up
    temp_s.used_block_bitmap_offset = 4;
    temp_s.inode_metadata_blocks = 1; // fits perfectly into one block
    temp_s.inode_metadata_offset = 3;
    temp_s.used_inode_bitmap_offset = 2;
    temp_s.used_inode_bitmap_count = MAX_FILE_COUNT;
    temp_s.directory_offset = 1;
    temp_s.directory_blocks = sizeof(dir_entry) * MAX_FILE_COUNT;
    temp_s.data_offset = 6;

    // write to disk
    memset(buffer, 0, BLOCK_SIZE);
    memcpy((void *)buffer, (void *)&temp_s, sizeof(super_block));
    if (block_write(0, buffer) == -1) {
        return -1;
    }
    
    // ***************** 1: initialize directory entry d *****************
    dir_entry temp_d;
    temp_d.is_used = false;
    temp_d.inode_number = -1;
    memset(temp_d.name, '\0', MAX_FILENAME_SIZE);
    temp_d.name[0] = '\0';
    temp_d.refcount = 0;

    memset(buffer, 0, BLOCK_SIZE);
    for (int it = 0; it < MAX_FILE_COUNT; it++) {
        memcpy((void *)(buffer + (it * sizeof(dir_entry))), (void *)&temp_d, sizeof(dir_entry));
    }
    if (block_write(temp_s.directory_offset, buffer) == -1) {
        return -1;
    }

    // ***************** 2: initialize inode bitmap *****************
    char temp_ib = '0';
    for (int it = 0; it < MAX_FILE_COUNT; it++) {
        memcpy((void *)buffer + it, (void *)&temp_ib, sizeof(char));
    }
    if (block_write(temp_s.inode_metadata_offset, buffer) == -1) {
        return -1;
    }
    
    // ***************** 3: initialize inode table *****************
    // inode for initialization
    inode temp_i;
    temp_i.direct_offset = -1;
    temp_i.single_indirect_offset = -1;
    temp_i.double_indirect_offset = -1;
    temp_i.file_size = 0;

    for (int it = 0; it < MAX_FILE_COUNT; it++) {
        memcpy((void *)(buffer + it * sizeof(inode)), (void *)&temp_i, sizeof(inode));
    }
    if (block_write(temp_s.used_inode_bitmap_offset, buffer) == -1) {
        return -1;
    }

    // ***************** 4,5: initialize data bitmap used_block_bitmap *****************
    char temp_db = '0';
    for (int it = 0; it < DISK_BLOCKS / 2; it++) {
        memcpy((void *)buffer + it * sizeof(char), (void *)&temp_db, sizeof(char));
    }
    if (block_write(4, buffer) == -1) {
        return -1;
    }
    if (block_write(5, buffer) == -1) {
        return -1;
    }

    // close disk 
    if (close_disk(disk_name) == -1) {
        return -1;
    }

    // return 0 on success
    return 0;
}

int mount_fs(const char *disk_name) {
    // open the disk and then load the meta-information 
    // that is necessary to handle the file system operations 
    // that are discussed below. The function returns 0 on success, 
    // and -1 when the disk disk_name could not be opened or when 
    // the disk does not contain a valid file system

    // open disk to write
    if(!disk_name) {
        return -1;
    }
    if (open_disk(disk_name) == -1) {
        return -1;
    }
    if(mounted) {
        return -1;
    }

    // load meta-information

    // create buffer
    char buffer[BLOCK_SIZE];

    // read in super block
    s = (super_block *)malloc(sizeof(super_block));

    if(block_read(0,buffer) == -1) {
        return -1;
    }
    memcpy((void *)s, (void *)buffer, sizeof(super_block));

    // read in dir_entry
    memset(buffer, 0, BLOCK_SIZE);
    if(block_read(1, buffer) == -1) {
        return -1;
    }
    memcpy((void *)d, (void *)buffer, sizeof(dir_entry) * MAX_FILE_COUNT);

    mounted = true;
    return 0;
}

int umount_fs(const char *disk_name) {
    if(!mounted) {
        return -1;
    }

    char buf[BLOCK_SIZE];
    memset(buf, 0, BLOCK_SIZE);

    // write metadata
    // superblock
    memcpy((void *)buf, (void *)s, sizeof(super_block));
        if (block_write(0, buf) == -1) {
        return -1;
    }
    
    // dir entry
    memset(buf, 0, BLOCK_SIZE);
    for (int it = 0; it < MAX_FILE_COUNT; it++) {
        memcpy((void *)buf + it * sizeof(dir_entry), (void *)&d, sizeof(dir_entry));
    }
    if (block_write(s->directory_offset, buf) == -1) {
        return -1;
    }
    
    // inode bitmap
    memset(buf, 0, BLOCK_SIZE);
    for (int it = 0; it < s->used_inode_bitmap_count / sizeof(char); it++) {
        memcpy((void *)buf, (void *)&used_inode_bitmap + it, sizeof(char));
    }
    if (block_write(s->inode_metadata_offset, buf) == -1) {
        return -1;
    }

    // inode table
    memset(buf, 0, BLOCK_SIZE);
    for (int it = 0; it < BLOCK_SIZE / sizeof(inode); it++) {
        memcpy((void *)buf, (void *)&i, sizeof(inode));
    }
    if (block_write(s->inode_metadata_offset, buf) == -1) {
        return -1;
    }

    // data bitmap
    memset(buf, 0, BLOCK_SIZE);
    for (int it = 0; it < s->used_block_bitmap_count / sizeof(char); it++) {
        memcpy((void *)buf, (void *)&used_block_bitmap, sizeof(char));
    }
    if (block_write(s->inode_metadata_offset, buf) == -1) {
        return -1;
    }

    if (close_disk(disk_name) == -1) {
        return -1;
    }

    for (int it = 0; it < MAX_FD_COUNT; it++) {
        fdt[it].is_used = false;
        fdt[it].inode_number = -1;
        fdt[it].offset = -1;
    }

    for (int it = 0; it < 5; it++) {
        used_block_bitmap[it] = '1';
    }

    free(s);
    mounted = false;
    return 0;
}

/**********************************************************************************************/
// FS FUNCTIONS
int fs_open(const char *name) {
    // check to see if there is empty fd
    int iter = 0;
    while (iter < MAX_FD_COUNT) {
        if (!fdt[iter].is_used) {
            break;
        }
        iter++;
    }
    if (iter == MAX_FD_COUNT) {
        return -1;
    }

    // check to see if it exits in directory
    int iterator = 0;
    while (iterator < MAX_FILE_COUNT) {
        if (strcmp(name, d[iterator].name) == 0) {
            break;
        }
        iterator++;
    }
    if (iterator == MAX_FILE_COUNT) {
        return -1;
    }
    if (!d[iterator].is_used) {
        return -1;
    }

    bool matched = false;
    // check directory entry for matching file name
    for (int it = 0; it < MAX_FILE_COUNT; it++) {
        if (strcmp(name, d[it].name) == 0) {
            fdt[iter].is_used = true;
            fdt[iter].inode_number = d[it].inode_number;
            fdt[iter].offset = 0;
            d[it].refcount++;
            matched = true;
        }
    }
    if (matched == false) {
        return -1;
    }
    return 0;
}

int fs_close(int fildes) {
    if(!(fildes > 0) && !(fildes < MAX_FD_COUNT)) {
        return -1;
    }
    if(fdt[fildes].is_used == false) {
        return -1;
    }
    if(fdt[fildes].is_used == true) {
        // fdt[fildes].inode_number = -1;
        // fdt[fildes].offset = -1;
        fdt[fildes].is_used = false;
    }
    return 0;
}
int fs_create(const char *name) {
    // check if the name is too long
    if(strlen(name) > MAX_FILENAME_SIZE) {
        return -1;
    }

    // check if name already exists
    for (int it = 0; it < MAX_FILE_COUNT; it++) {
        if (strcmp(name, d[it].name) == 0) {
            return -1;
        }
    }
    // check for open space in file
    int index;
    for (int it = 0; it < MAX_FILE_COUNT; it++) {
        if (!d[it].is_used) {
            index = it;
            break;
        }
    }
    // fail if directory if full
    if (index == MAX_FILE_COUNT - 1) {
        return -1;
    }
    int iter = 0;
    while (iter < MAX_FILE_COUNT) {
        if (!used_inode_bitmap[iter]) {
            break;
        }
        iter++;
    }

    d[index].is_used = true;
    d[index].inode_number = iter;
    strcpy(d[index].name, name);
    d[index].refcount = 0;
    
    return 0;
}
int fs_delete(const char *name) {
    
    // seek through dir array
    int num_i = -1;
    int iter = 0;
    // bool kms = strcmp(name, d[iter].name);
    // printf("%d",kms);
    while (iter < MAX_FILE_COUNT) {
        if (strcmp(name, d[iter].name) == 0) {
            memset(d[iter].name, '\0', MAX_FILENAME_SIZE);
            // d[iter].inode_number = -1;
            d[iter].is_used = false;
            d[iter].refcount = 0;
            break;
        }
        iter++;
    }
    // return if name not found
    if (iter == MAX_FILE_COUNT) {
        return -1;
    }

    // fail if file is open
    int iterator = 0;
    while (iterator < MAX_FD_COUNT) {
        if(fdt[iterator].inode_number == iter) {
            break;
        }
        iterator++;
    }
    if (iterator != MAX_FD_COUNT && fdt[iterator].is_used) {
        return -1;
    }

    // free inode
    i[num_i].direct_offset = 0;
    i[num_i].file_size = 0; 

    return 0;
}
int fs_read(int fildes, void *buf, size_t nbyte) {
    if (fildes < 0 || fildes > MAX_FD_COUNT) {
        return -1;
    }
    if (nbyte < 0 || nbyte > MAX_FILE_SIZE) {
        return -1;
    }

    if (fdt[fildes].inode_number == -1) {
        return -1;
    }
    if (fdt[fildes].is_used == false) {
        return -1;
    }
    int num_i = fdt[fildes].inode_number;
    // find in directory
    int iter = 0;
    while (iter < MAX_FILE_COUNT) {
        if (d[iter].inode_number == num_i) {
            break;
        }
        iter++;
    }
    if (iter == MAX_FILE_COUNT) {
        return -1;
    }

    char buffer[BLOCK_SIZE];
    memset(buffer , 0, BLOCK_SIZE);
    // if (block_read(i[num_i].direct_offset, buffer) == -1) {
    //     return -1;
    // }
    if (nbyte <= BLOCK_SIZE) {
        if (block_read(i[num_i].direct_offset, buffer) == -1) {
            return -1;
        }
        memcpy(buf, buffer, nbyte);

        if (fdt[fildes].offset + nbyte > MAX_FILE_SIZE) {
            nbyte = MAX_FILE_SIZE - fdt[fildes].offset;
        }
    }

    return nbyte;
}
int fs_write(int fildes, void *buf, size_t nbyte) {
    // check for valid file descriptor
    if (fildes < 0 || fildes > MAX_FD_COUNT) {
        return -1;
    }
    // check that file is open 
    if (fdt[fildes].is_used == false) {
        return -1;
    }

    // check that inode numbers are equal and directory entry is found
    int iter = 0;
    while (iter < MAX_FILE_COUNT) {
        if (d[iter].is_used && fdt[fildes].inode_number == d[iter].inode_number) {
            break;
        }
        iter++;
    }
    if (iter == MAX_FILE_COUNT) {
        return -1;
    }

    // check if nbytes fit
    if (fdt[fildes].offset + nbyte > MAX_FILE_SIZE) {
        nbyte = MAX_FILE_SIZE - fdt[fildes].offset;
    }

    // update data structures
    // i[iter].direct_offset += nbyte;
    // set inode
    if (iter == 0) {
        i[iter].direct_offset = 6 * BLOCK_SIZE;
        i[iter].file_size += nbyte;
    }
    else {
        i[iter].direct_offset = i[iter].direct_offset + i[iter].file_size;
        i[iter].file_size += nbyte;
    }

    // write to disk
    if (nbyte <= BLOCK_SIZE) {
         // write to disk
        char buffer[BLOCK_SIZE];
        memset(buffer, 0, BLOCK_SIZE);
        memcpy(buffer, buf, nbyte);
        if (block_write(i[iter].direct_offset, buffer) == -1) {
            return -1;
        } 
    }
    else {
        char buffer[BLOCK_SIZE];
        char bufin[BLOCK_SIZE];
        memset(buffer, 0, BLOCK_SIZE);
        // read in last file if in same block
        if (block_read(i[iter].direct_offset / BLOCK_SIZE, bufin) == -1) {
            return -1;
        }
        // block we start writing to
        int blocknum = i[iter].direct_offset / BLOCK_SIZE;
        // temp, offset from block we start writing to
        int temp = i[iter].direct_offset - blocknum * BLOCK_SIZE;
        int endblock = BLOCK_SIZE - temp;
        memcpy(bufin + temp, buf, endblock);
        if (block_write(blocknum, bufin) == -1) {
            return -1;
        }
        int remaining = nbyte - endblock;
        int it = 1;
        int blocksleft = remaining / BLOCK_SIZE;
        while (blocksleft > 1) {
            memcpy(buffer, buf + BLOCK_SIZE * it, BLOCK_SIZE);
            if (block_write(blocknum + it, buffer) == -1) {
            return -1;
            }
            remaining -= BLOCK_SIZE;
            it++;
            blocksleft--;
        }

        if (remaining > 0) {
            memset(buffer, 0 , BLOCK_SIZE);
            memcpy(buffer, buf + BLOCK_SIZE * it, remaining);
            if (block_write(blocknum + it, buffer) == -1) {
                return -1;
            }
        }
    }

    return nbyte;
}
int fs_get_filesize(int fildes) {
    if (fildes < 0 || fildes > MAX_FD_COUNT) {
        return -1;
    }
    if (fdt[fildes].is_used == false) {
        return -1;
    }
    // get inode number
    if (fdt[fildes].inode_number == -1) {
        return -1;
    }
    int num_i = fdt[fildes].inode_number;
    return i[num_i].file_size;
}
int fs_listfiles(char ***files) {
    int iter = 0;
    *files = (char **)malloc(MAX_FILE_COUNT * sizeof(char *));
    for(int it = 0; it < MAX_FILE_COUNT; it++) {
        files[0][it] = (char *)malloc(MAX_FILENAME_SIZE * sizeof(char));
        if (d[it].is_used) {
            strcpy(files[0][iter],d[it].name);
            iter++;
        }
    }
    *files[0][iter] = '\0';
    return 0;
}
int fs_lseek(int fildes, off_t offset) {
//     This function sets the file pointer (the offset used for read and write operations) associated with
// the file descriptor fd to the argument offset. It is an error to set the file pointer beyond the end of 
// the file. To append to a file, one can set the file pointer to the end of a file, for example, by calling
//  fs_lseek(fd, fs_get_filesize(fd));. Upon successful completion, a value of 0 is returned. fs_lseek returns
//   -1 on failure. It is a failure when the file descriptor fd is invalid, when the requested offset is larger
//    than the file size, or when offset is less than zero. 

    // check if real fd
    if (fildes < 0 || fildes > MAX_FD_COUNT) {
        return -1;
    }
    if (!fdt[fildes].is_used) {
        return -1;
    }

    // fail if offset greater than file
    if (i[fdt[fildes].inode_number].file_size < offset) {
        return -1;
    }

    // fail if less than 0
    if (offset < 0) {
        return -1;
    }

    fdt[fildes].offset = offset;
    return 0;
}
int fs_truncate(int fildes, off_t length) {

    // This function causes the file referenced by fd to be truncated to length bytes
    //  in size. If the file was previously larger than this new size, the extra data
    //   is lost and the corresponding data blocks on disk (if any) must be freed. It 
    //   is not possible to extend a file using fs_truncate. When the file pointer is 
    //   larger than the new length, then it is also set to length (the end of the file). 
    //   Upon successful completion, a value of 0 is returned. fs_lseek returns -1 on 
    //   failure. It is a failure when the file descriptor fd is invalid or the requested 
    //   length is larger than the file size. 

    // error check fd
    if (fildes < 0 || fildes > MAX_FD_COUNT) {
        return -1;
    }
    // check if fd used
    if (!fdt[fildes].is_used) {
        return -1;
    }
    // check to see if fd open
    if (fdt[fildes].inode_number == -1) {
        return -1;
    }
    // get inode number
    int num_i = fdt[fildes].inode_number;
    // find corresponding file in directory
    int iter = 0; // index of directory entry
    while (iter < MAX_FILE_COUNT) {
        if (d[iter].inode_number == num_i) {
            break;
        }
        iter++; 
    }
    if (iter == MAX_FILE_COUNT) {
        return -1;
    }
    char buffer[BLOCK_SIZE];
    memset(buffer , 0, BLOCK_SIZE);
    int new_s = (int)length;

    if (new_s < 0) {
        return -1;
    }
    if (new_s > i[num_i].file_size) {
        return -1;
    }
    if (block_read((i[num_i].direct_offset / BLOCK_SIZE), buffer) == -1) {
        return -1;
    } 
    memset(buffer + new_s, 0, i[num_i].file_size - new_s);
    i[num_i].file_size = new_s;
    if (block_read((i[num_i].direct_offset / BLOCK_SIZE), buffer) == -1) {
        return -1;
    } 
    return 0;
}