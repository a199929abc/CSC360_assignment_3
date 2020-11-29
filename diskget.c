#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "basic.h"


void display_dir_sec(DirSec dirsec){
    for (int i = 0; i < dirsec.size; i++){
        // if (strcmp(simp_filename(dirsec.items[i].filename), ".") == 0
        //     ||strcmp(simp_filename(dirsec.items[i].filename), "..") == 0)
        //     continue;
        if (dirsec.items[i].is_file)
            printf("F %10d %s.%s %s\n", dirsec.items[i].file_size, 
            simp_filename(dirsec.items[i].filename), simp_filename(dirsec.items[i].ext),
            make_datetime(dirsec.items[i].c_date, dirsec.items[i].c_time));
        else
            printf("D %10d %s %s\n", dirsec.items[i].file_size, dirsec.items[i].filename,
            make_datetime(dirsec.items[i].c_date, dirsec.items[i].c_time));
    }
}

void dfs(FILE *fp, Item item, char* prefix){
    if (item.is_file)
        return;
    char new_prefix[BUFF_SIZE];
    sprintf(new_prefix, "%s/%s", prefix, simp_filename(item.filename));
    printf("%s\n==================\n", new_prefix);
    int entry = get_fat_entry(fp, item.start_cluster);
    DirSec dirsec = parse_dir_sector(read_sector_n(fp, 33 + item.start_cluster - 2));
    display_dir_sec(dirsec);
    while(0 < entry && entry < 0xFF0){
        dirsec = parse_dir_sector(read_sector_n(fp, 33 + entry - 2));
        display_dir_sec(dirsec);
        entry = get_fat_entry(fp, entry);
    }

    dirsec = parse_dir_sector(read_sector_n(fp, 33 + item.start_cluster - 2));
    for (int i = 0; i < dirsec.size; i++){
        dfs(fp, dirsec.items[i], new_prefix);
    }
    while(0 < entry && entry < 0xFF0){
        dirsec = parse_dir_sector(read_sector_n(fp, 33 + entry - 2));
        for (int i = 0; i < dirsec.size; i++){
            dfs(fp, dirsec.items[i], new_prefix);
        }
        entry = get_fat_entry(fp, entry);
    }
}

void fetch_file_to_cur_dir(FILE *disk_fp, Item item, char *filename){
    FILE *outfp = fopen(filename, "wb");
    int remain_size = item.file_size;
    if (outfp){
        char *sec = NULL;
        // printf("%d ", item.start_cluster);
        int entry = get_fat_entry(disk_fp, item.start_cluster);
        // printf("%d ", item.start_cluster);
        sec = read_sector_n(disk_fp, 33 + item.start_cluster - 2);
        // printf("%d ", entry);
        // int n;
        if (remain_size > SECTOR_SIZE){
            fwrite(sec, 1, SECTOR_SIZE, outfp);
            // printf("%d\n", n);
            remain_size -= SECTOR_SIZE;
        }else{
            fwrite(sec, 1, remain_size, outfp);
            // printf("%d\n", n);
            remain_size -= remain_size;
        }
        while(0 < entry && entry < (uint32_t)0xFF0){
            sec = read_sector_n(disk_fp, 33 + entry - 2);
            if (remain_size > SECTOR_SIZE){
                fwrite(sec, 1, SECTOR_SIZE, outfp);
                //  printf("%d\n", n);
                remain_size -= SECTOR_SIZE;
            }else{
                fwrite(sec, 1, remain_size, outfp);
                // printf("%d\n", n);
                remain_size -= remain_size;
            }
            entry = get_fat_entry(disk_fp, entry);
            // printf("entry %d remain %d\n", entry, remain_size);
        }
        fclose(outfp);
    }else
        printf("open %s error\n", filename);
}


int main(int argc, char *argv[])
{
    char diskname[BUFF_SIZE];
    char filename[BUFF_SIZE];
	if (argc == 3){
        strcpy(diskname, argv[1]);
        str_to_upper(argv[2]);
        FILE *fp = fopen(diskname, "rb");
        if (fp){
            char prefix[BUFF_SIZE];
            memset(prefix, 0, sizeof(prefix));
            for (int i = 19; i <= 32;i++){
                DirSec dirsec = parse_dir_sector(read_sector_n(fp, i));
                /*
                find the target filename
                */
                for (int j = 0; j < dirsec.size;j++){
                    if (dirsec.items[j].is_file){
                        sprintf(filename, "%s.%s", simp_filename(dirsec.items[j].filename), simp_filename(dirsec.items[j].ext));
                        if (strcmp(filename, argv[2]) == 0){
                            fetch_file_to_cur_dir(fp, dirsec.items[j], filename);
                        }
                    }
                }
            }

        }
    }else{
        printf("wrong arg number\n");
    }
	return 0;
}
