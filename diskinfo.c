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

char os_name[BUFF_SIZE], disk_label[BUFF_SIZE];


int find_all_files(FILE *fp, int sector_index){
    char *sec = read_sector_n(fp, sector_index);
    for (int i = 0; i < MAX_ITEM_IN_SECTOR; i++){
        if (sec[i * ITEM_SIZE + 11] == 0x08)
            strcpy(disk_label, &sec[i * ITEM_SIZE]);
    }
    DirSec dirsec = parse_dir_sector(sec);
    int res = 0;
    for (int i = 0; i < dirsec.size; i++){
        Item item = dirsec.items[i];
        if (item.is_file)
            res += 1;
        else
            res += find_all_files(fp, 33 + item.start_cluster - 2);
    }
    return res;
}

int main(int argc, char *argv[])
{
    char filename[BUFF_SIZE];
	if (argc == 2){
        strcpy(filename, argv[1]);
        FILE *fp = fopen(filename, "rb");
        if (fp){
            char *boot_sec = read_sector_n(fp, 0);
            int bytes_per_sector = bytes_to_int(&boot_sec[11], 2);
            int total_sec_count = bytes_to_int(&boot_sec[19], 2);
            int fat_copies = bytes_to_int(&boot_sec[16], 1);
            int sec_per_fat = bytes_to_int(&boot_sec[22], 2);

            char all_fat_bytes[SECTOR_SIZE * FAT_SECTOR_NUM];
            for (int i = 0; i< FAT_SECTOR_NUM ; i++){
                memcpy(&all_fat_bytes[SECTOR_SIZE * i], read_sector_n(fp, i + 9), SECTOR_SIZE);
            }
            int fat_entry = 0;
            int unused = 0;
            for (int i = 0; i <= 2848; i++){
                if (i % 2 == 0)
                    fat_entry = (all_fat_bytes[1 + 3 * i / 2] & 0x0f) | all_fat_bytes[3 * i / 2];
                else
                    fat_entry = (all_fat_bytes[3 * i / 2] >> 4) | all_fat_bytes[1 + 3 * i / 2];
                if (fat_entry == 0x00 || fat_entry == 0x01)
                    unused += 1;
            }
            int cnt_files = 0;
            for (int i = 19; i <= 32;i++){
                cnt_files += find_all_files(fp, i);
            }
            strncpy(os_name, &boot_sec[3], 8);
            printf("OS Name: %s\n", os_name);
            printf("Label of the disk: %s\n", disk_label);
            printf("Total size of the disk: %d\n", total_sec_count * bytes_per_sector);
            printf("Free size of the disk: %d\n\n", unused * bytes_per_sector);
            printf("=============\n");
            printf("The number of files in the disk: %d\n\n", cnt_files);
            printf("=============\n");
            printf("Number of FAT copies: %d\n", fat_copies);
            printf("Sectors per FAT: %d\n", sec_per_fat);
        }
    }else{
        printf("wrong arg number\n");
    }
	return 0;
}
