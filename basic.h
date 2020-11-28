#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#define BUFF_SIZE 128
#define SECTOR_SIZE 512
#define MAX_ITEM_IN_SECTOR 16
#define ITEM_SIZE 32
#define FAT_ENTRY_NUM_PER_SECTOR 341
#define FAT_SECTOR_NUM 9
#define MAX_FILENAME_LEN 8
#define MAX_EXT_LEN 3
#define MAX_DATA_CLUSTER 2848
typedef struct Item{
    char filename[MAX_FILENAME_LEN];
    char ext[MAX_EXT_LEN];
    int is_file;
    int start_cluster;
    int file_size;
    int c_date;
    int c_time;
} Item;
typedef struct DirSec{
    int size;
    Item items[16];
} DirSec;

typedef struct Paths{
    int size;
    char path[BUFF_SIZE][BUFF_SIZE];
} Paths;

char* read_sector_n(FILE *fp, int n);
DirSec parse_dir_sector(char *bytes);
int bytes_to_int(char *bytes, int size);
char *int_to_bytes(int num);
int get_fat_entry(FILE *fp, int index);
void set_fat_entry(FILE *fp, int index, int num);
char *simp_filename(char *filename);
char *simp_ext(char *ext);
void get_current_datetime(uint16_t *cdate, uint16_t *ctime);
char *make_datetime(int date, int time);
Paths parse_path(char *path, char sep[]);
int get_available_item_index(char *bytes);
void get_filename_and_ext(char *full_filename, char *filename, char *ext);
void str_to_upper(char *p);