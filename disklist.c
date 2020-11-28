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
            simp_filename(dirsec.items[i].filename), simp_ext(dirsec.items[i].ext),
            make_datetime(dirsec.items[i].c_date, dirsec.items[i].c_time));
        else
            printf("D %10d %s %s\n", dirsec.items[i].file_size, dirsec.items[i].filename,
            make_datetime(dirsec.items[i].c_date, dirsec.items[i].c_time));
    }
}

void dfs(FILE *fp, Item item, char* prefix){
    if (item.is_file)
        return;
    // if (strcmp(simp_filename(item.filename), ".") == 0
    //         ||strcmp(simp_filename(item.filename), "..") == 0)
    //         return;
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


int main(int argc, char *argv[])
{
    char filename[BUFF_SIZE];
	if (argc == 2){
        strcpy(filename, argv[1]);
        FILE *fp = fopen(filename, "rb");
        if (fp){
            char prefix[BUFF_SIZE];
            memset(prefix, 0, sizeof(prefix));
            printf("Root\n==================\n");
            for (int i = 19; i <= 32;i++){
                DirSec dirsec = parse_dir_sector(read_sector_n(fp, i));
                display_dir_sec(dirsec);
            }
            for (int i = 19; i <= 32;i++){
                DirSec dirsec = parse_dir_sector(read_sector_n(fp, i));
                for(int j = 0; j < dirsec.size; j++){
                    dfs(fp, dirsec.items[j], prefix);
                }
            }

        }
    }else{
        printf("wrong arg number\n");
    }
	return 0;
}
