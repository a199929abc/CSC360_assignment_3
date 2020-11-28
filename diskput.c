#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include "basic.h"

int min(int x, int y){
    return x < y ? x : y;
}
void parse_file_path(char *full_path, char *dir_path, char *filename){
    int slash_pos = -1;
    for (int i = 0; i < strlen(full_path); i++){
        if (full_path[i] == '/')
            slash_pos = i;
    }
    if (slash_pos == -1){
        strcpy(filename, full_path);
        // printf("%s %s", full_path, filename);
    }else{
        strncpy(dir_path, &full_path[0], slash_pos);
        strcpy(filename, &full_path[slash_pos + 1]);
    }
}

int get_unused_entry_num(FILE *diskfp){
    int res = 0;
    for (int i = 2; i <= MAX_DATA_CLUSTER; i++){
        if (get_fat_entry(diskfp, i) == 0x000)
            res += 1;
    }
    return res;
}

size_t write_item_to_sector(FILE *diskfp, char *item, int sector_index, int item_index){
    int offset = sector_index * SECTOR_SIZE + item_index * ITEM_SIZE;
    fseek(diskfp, offset, SEEK_SET);
    return fwrite(item, ITEM_SIZE, 1, diskfp);
}

size_t write_sector(FILE *diskfp, char *item, int sector_index){
    int offset = sector_index * SECTOR_SIZE;
    fseek(diskfp, offset, SEEK_SET);
    return fwrite(item, SECTOR_SIZE, 1, diskfp);
}


// start cluster = -1 indicate search from root
int check_dir_exists(FILE *diskfp, Paths path, int depth, int start_cluster){
    if (path.size == 0)
        return 0;
    if (depth >= path.size)
        return start_cluster;
    
    // printf("%s %d\n", path.path[depth], start_cluster);

    if (start_cluster == -1){
        for (int i = 19; i <= 32;i++){
            DirSec dirsec = parse_dir_sector(read_sector_n(diskfp, i));
            for (int j = 0; j < dirsec.size; j++){
                if (!dirsec.items[j].is_file && strcmp(simp_filename(dirsec.items[j].filename), path.path[depth]) == 0)
                    return check_dir_exists(diskfp, path, depth + 1, dirsec.items[j].start_cluster);
            }
        }
    }else if (start_cluster >= 2){
        int entry = start_cluster;
        DirSec dirsec;
        while(0 < entry && entry < 0xFF0){
            dirsec = parse_dir_sector(read_sector_n(diskfp, 33 + entry - 2));
            for (int j = 0; j < dirsec.size; j++){
                if (!dirsec.items[j].is_file && strcmp(simp_filename(dirsec.items[j].filename), path.path[depth]) == 0)
                    return check_dir_exists(diskfp, path, depth + 1, dirsec.items[j].start_cluster);
            }
            entry = get_fat_entry(diskfp, entry);
         }
    }
    return -1;
}

int check_file_exists(FILE *diskfp, int startcluster, char *filename, char *ext){
    if (startcluster == 0){
        for (int i = 19; i <= 32;i++){
            DirSec dirsec = parse_dir_sector(read_sector_n(diskfp, i));
            for (int j = 0; j < dirsec.size; j++){
                if (strcmp(simp_filename(dirsec.items[j].filename), filename) == 0 && strcmp(simp_ext(dirsec.items[j].ext), ext) == 0)
                    return 1;
            }
        }
    }else{
        int entry = startcluster;
        while(0 < entry && entry < 0xFF0){
            DirSec dirsec = parse_dir_sector(read_sector_n(diskfp, 33 + entry - 2));
            for (int j = 0; j < dirsec.size; j++){
                if (strcmp(simp_filename(dirsec.items[j].filename), filename) == 0 && strcmp(simp_ext(dirsec.items[j].ext), ext) == 0)
                    return 1;
            }
            entry = get_fat_entry(diskfp, entry);
        }
    }
    return 0;
}


void write_file(FILE *diskfp, FILE *localfp,
    char *filename, char *ext, int filesize,
    int startcluster){
    int unused_sectors[MAX_DATA_CLUSTER];
    int unused_size = 0;
    char meta[ITEM_SIZE];
    memset(meta, 0, sizeof(meta));
    memset(unused_sectors, 0, sizeof(unused_sectors));

    int find = 0;
    for (size_t i = 0; i < MAX_EXT_LEN + MAX_FILENAME_LEN; i++)
        meta[i] = 0x20;

    for (int i = 0; i < MAX_FILENAME_LEN && filename[i] != 0x00; i++){
        meta[i] = filename[i];
    }
    for (int i = 0; i < MAX_EXT_LEN && ext[i] != 0x00; i++){
        meta[i + MAX_FILENAME_LEN] = ext[i];
    }
    uint16_t cdate, ctime;
    get_current_datetime(&cdate, &ctime);
    strncpy(&meta[16], int_to_bytes(cdate), 2);
    strncpy(&meta[14], int_to_bytes(ctime), 2);

    // printf("%x\t%x\n", cdate, ctime);
    // char *p = make_datetime(cdate, ctime);
    // printf("%s\n", p);
    
    strncpy(&meta[28], int_to_bytes(filesize), 4);
    meta[11] = 0x80; 

    for (int i = 2; i <= MAX_DATA_CLUSTER; i++){
        if (get_fat_entry(diskfp, i) == 0x000)
            unused_sectors[unused_size++] = i;
    }
    // printf("%d\n", startcluster);

    if (startcluster == 0){
        int index_sector = -1, index_item = -1;
        for (int i = 19; i <= 32;i++){
            int index = get_available_item_index(read_sector_n(diskfp, i));
            if (index >= 0){
                index_sector = i;
                index_item = index;
                find = 1;
                break;
            }
        }
        if (!find){
            printf("Excced Max File Number limit\n");
        }else{
            int remain_size = filesize;
            char buff[SECTOR_SIZE];
            int cnt = 0;
            while (remain_size > 0){
                memset(buff, 0, sizeof(buff));
                fread(buff, min(SECTOR_SIZE, remain_size), 1, localfp);
                write_sector(diskfp, buff, 33 + unused_sectors[cnt] - 2);
                remain_size -= SECTOR_SIZE;
                if (remain_size > 0){
                    set_fat_entry(diskfp, unused_sectors[cnt], unused_sectors[cnt + 1]);
                }else {
                    set_fat_entry(diskfp, unused_sectors[cnt], 0xFFF);
                }
                cnt += 1;
            }
            strncpy(&meta[26], int_to_bytes(unused_sectors[0]), 2);
            write_item_to_sector(diskfp, meta, index_sector, index_item);
        }
    }else{

        int entry = startcluster;
        int index_sector = -1, index_item = -1;
        int index;
        while(0 < entry && entry < 0xFF0){
            // printf("find...\n");
            index = get_available_item_index(read_sector_n(diskfp, 33 + entry - 2));
            // printf("%d index test\n", index);
            if (index >= 0){
                index_sector = entry;
                index_item = index;
                find = 1;
                break;
            }
            entry = get_fat_entry(diskfp, entry);
        }
        if (!find){
            printf("Excced Max File Number limit\n");
        }else{

            int remain_size = filesize;
            char buff[SECTOR_SIZE];
            int cnt = 0;
            while (remain_size > 0){
                memset(buff, 0, sizeof(buff));
                fread(buff, min(SECTOR_SIZE, remain_size), 1, localfp);
                write_sector(diskfp, buff, 33 + unused_sectors[cnt] - 2);
                remain_size -= SECTOR_SIZE;
                if (remain_size > 0){
                    set_fat_entry(diskfp, unused_sectors[cnt], unused_sectors[cnt + 1]);
                }else {
                    set_fat_entry(diskfp, unused_sectors[cnt], 0xFFF);
                }
                cnt += 1;
            }
                            

            strncpy(&meta[26], int_to_bytes(unused_sectors[0]), 2);
            write_item_to_sector(diskfp, meta, index_sector, index_item); 

        }

    }
    // printf("writing to ... test\n");
}
int main(int argc, char *argv[])
{
    char diskname[BUFF_SIZE];
    char filepath[BUFF_SIZE], localfile[BUFF_SIZE];
    char buff[BUFF_SIZE];
	if (argc == 3){
        strcpy(diskname, argv[1]);
        strcpy(filepath, argv[2]);
        str_to_upper(filepath);
        FILE *fp = fopen(diskname, "rb+");
        if (fp){
            memset(buff, 0, sizeof(buff));
            if (filepath[0] != '/'){
                buff[0] = '/';
                strcpy(&buff[1], filepath);
            }else
                strcpy(&buff[0], filepath);
            Paths fullpath = parse_path(buff, "/");
            Paths dirpath = fullpath;
            char filename[FILENAME_MAX], ext[MAX_EXT_LEN];
            memset(localfile, 0, sizeof(localfile));
            strcpy(localfile, fullpath.path[fullpath.size - 1]);
            memset(filename, 0, sizeof(filename));
            memset(ext, 0, sizeof(ext));
            get_filename_and_ext(localfile, filename, ext);
            dirpath.size -= 1;
            int ret = check_dir_exists(fp, dirpath, 0, -1);
            if (ret >= 0){
                // printf("%s\t%s\n", filename, ext);
                if (!check_file_exists(fp, ret, filename, ext)){
                    FILE *localfp = fopen(localfile, "rb");
                    if(!localfp)
                        printf("File not found\n");
                    else{
                        fseek(localfp, 0, SEEK_END);
                        int filesize = ftell(localfp);
                        fseek(localfp, 0, SEEK_SET);
                        if (get_unused_entry_num(fp) * SECTOR_SIZE >= filesize){
                            write_file(fp, localfp, filename, ext, filesize, ret);
                            fclose(fp);
                            fclose(localfp);
                        }else
                            printf("No enough free space in the disk image\n");
                    }
                }else
                    printf("The filename %s existed\n", localfile);
            }else
                printf("The directory not found\n");
        }
    }else{
        printf("wrong arg number\n");
    }
	return 0;
}
