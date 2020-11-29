#include "basic.h"

char* read_sector_n(FILE *fp, int n){
    char *res = (char *)malloc(SECTOR_SIZE * sizeof(char));
    fseek(fp, SECTOR_SIZE * n, SEEK_SET);
    fread(res, SECTOR_SIZE, 1, fp);
    return res;
}
int bytes_to_int(char *bytes, int size){
    int res = 0;
    for (int i = 0; i < size; i++){
        res = res << 8;
        res |= (uint8_t)bytes[size - i - 1];
        // printf("%d ", bytes[i]);
    }
    return res;
}
char *int_to_bytes(int num){
    char *buff = (char *)malloc(sizeof(char) * BUFF_SIZE);
    memset(buff, 0, sizeof(char) * BUFF_SIZE);
    int cnt = 0;
    while (num){
        uint8_t byte = num & 0xff;
        buff[cnt++] = byte;
        num = num >> 8;
    }
    return buff;
}
char *simp_filename(char *filename){
    char *res = (char *)malloc(sizeof(char) * MAX_FILENAME_LEN);
    memset(res, 0, sizeof(char)* MAX_FILENAME_LEN);
    for (int j = 0; j < MAX_FILENAME_LEN && filename[j] != 0x20; j++)
        res[j] = filename[j];
    return res;
}
char *simp_ext(char *ext){
    char *res = (char *)malloc(sizeof(char) * MAX_EXT_LEN);
    memset(res, 0, sizeof(char)* MAX_EXT_LEN);
    for (int j = 0; j < MAX_EXT_LEN && ext[j] != 0x20; j++)
        res[j] = ext[j];
    return res;
}
void get_filename_and_ext(char *full_filename, char *filename, char *ext){
    int pos = -1;
    for(int i = 0; i < strlen(full_filename); i++){
        if (full_filename[i] == '.')
            pos = i;
    }
    if (pos != -1){
        strncpy(filename, full_filename, pos);
        strncpy(ext, &full_filename[pos + 1], strlen(full_filename) - pos - 1);
    }else
        strncpy(filename, full_filename, strlen(full_filename));
}
DirSec parse_dir_sector(char *bytes){
    char item_bytes[ITEM_SIZE];
    DirSec dirsec;
    dirsec.size = 0;
    for (int i = 0; i < MAX_ITEM_IN_SECTOR; i++){
        memcpy(item_bytes, &bytes[i * ITEM_SIZE], ITEM_SIZE);
        Item item;
        memset(&item, 0, sizeof(Item));
        strncpy(&item.filename[0], &item_bytes[0], 8);
        // for (int j = 0; j < 8 && item_bytes[j] != 0x20; j++)
        //     item.filename[j] = item_bytes[j];
        strncpy(&item.ext[0], &item_bytes[8], 3);
        item.start_cluster = bytes_to_int(&item_bytes[26], 2);
        item.file_size = bytes_to_int(&item_bytes[28], 4);
        item.c_date = (uint16_t)bytes_to_int(&item_bytes[16], 2);
        item.c_time = (uint16_t)bytes_to_int(&item_bytes[14], 2);
        char attr = item_bytes[11];
        item.is_file = attr & 0x10 ? 0 : 1;
        if (item.filename[0]!= 0x00 && (unsigned char)item.filename[0]!= 0xe5
            && !(attr & 0x0f) && !(attr & 0x08)){
            if (strcmp(simp_filename(item.filename), ".") == 0
                ||strcmp(simp_filename(item.filename), "..") == 0)
                    continue;
            // printf("%s\n", simp_filename(item.filename));
            // printf("%s\n", item.ext);
            dirsec.items[dirsec.size++] = item;
        }
    }
    return dirsec;
} 

int get_available_item_index(char *bytes){
    char item_bytes[ITEM_SIZE];
    // printf("a\n");
    for (int i = 0; i < MAX_ITEM_IN_SECTOR; i++){
        // printf("%d\n", i);
        memcpy(item_bytes, &bytes[i * ITEM_SIZE], ITEM_SIZE);
        char attr = item_bytes[11];
        if ((item_bytes[0] == 0x00 || (uint8_t)item_bytes[0] == 0xe5) && !(attr & 0x0f) && !(attr & 0x08)){
            return i;
        }
    }
    return -1;
}

int get_fat_entry(FILE *fp, int index){
    uint8_t a = 0, b = 0;
    if (index % 2 == 0){
        fseek(fp, 0x0000200 + 1 + 3 * index / 2, SEEK_SET);
        fread(&a, 1, 1,fp);
        fseek(fp, 0x0000200 + 3 * index / 2, SEEK_SET);
        fread(&b, 1, 1,fp);
        return ((a & 0x0f) << 8) | b;
    }else{
        fseek(fp, 0x0000200 + 3 * index / 2, SEEK_SET);
        fread(&a, 1, 1,fp);
        fseek(fp, 0x0000200 + 1 + 3 * index / 2, SEEK_SET);
        fread(&b, 1, 1,fp);
        return (a >> 4) | (b << 4);
    }
}

void set_fat_entry(FILE *fp, int index, int num){
    uint8_t a = 0, b = 0, tmp = 0;
    if (index % 2 == 0){
        a = (num & 0xf00) >> 8;
        fseek(fp, 0x0000200 + 1 + 3 * index / 2, SEEK_SET);
        fread(&tmp, 1, 1, fp);
        a = (tmp & 0xf0) | a;
        fseek(fp, 0x0000200 + 1 + 3 * index / 2, SEEK_SET);
        fwrite(&a, 1, 1, fp);

        b = (num & 0xff);
        fseek(fp, 0x0000200 + 3 * index / 2, SEEK_SET);
        fwrite(&b, 1, 1,fp);
    }else{
        a = (num & 0x0f) << 4;
        fseek(fp, 0x0000200 + 3 * index / 2, SEEK_SET);
        fread(&tmp, 1, 1, fp);
        a = a | (tmp & 0x0f);
        fseek(fp, 0x0000200 + 3 * index / 2, SEEK_SET);
        fwrite(&a, 1, 1, fp);

        b = (num & 0xff0) >> 4;
        fwrite(&b, 1, 1, fp);
    }
}

void get_current_datetime(uint16_t *cdate, uint16_t *ctime){
    time_t timep;
    struct tm *p;
    time(&timep);
    p=gmtime(&timep);

    *cdate = 0;
    *cdate |= ((p->tm_year - 80) << 9);
    *cdate |= ((p->tm_mon + 1) << 5);
    *cdate |= (p->tm_mday);
    // printf("%d\t%d\t%d\n", p->tm_year, p->tm_mon + 1, p->tm_mday);
    *ctime = 0;
    *ctime |= ((p->tm_hour + 8) << 11);
    *ctime |= (p->tm_min << 5);
    // printf("%d\t%d\n", p->tm_hour + 8, p->tm_min);
}

char *make_datetime(int date, int time){
    int year = (date & 0xFE00) >> 9;
    int month = (date & 0x1E0) >> 5;
    int day = date & 0x1F;

    int hour = (time & 0xF800) >> 11;
    int minute = (time & 0x7E0) >> 5;
    char *res = (char *)malloc(sizeof(char) * BUFF_SIZE);
    memset(res, 0, sizeof(char) * BUFF_SIZE);
    char month_abb[BUFF_SIZE];
    switch (month)
    {
        case 1:strcpy(month_abb, "Jan");break;
        case 2:strcpy(month_abb, "Feb");break;
        case 3:strcpy(month_abb, "Mar");break;
        case 4:strcpy(month_abb, "Apr");break;
        case 5:strcpy(month_abb, "May");break;
        case 6:strcpy(month_abb, "Jun");break;
        case 7:strcpy(month_abb, "Jul");break;
        case 8:strcpy(month_abb, "Aug");break;
        case 9:strcpy(month_abb, "Sep");break;
        case 10:strcpy(month_abb, "Oct");break;
        case 11:strcpy(month_abb, "Nov");break;
        case 12:strcpy(month_abb, "Dec");break;
        default:break;
    }
    sprintf(res, "%s %d %d %d:%d", month_abb, day, year + 1980, hour, minute);
    return res;
}

Paths parse_path(char *path, char sep[]){
    char *token;
    Paths paths;
    memset(&paths, 0, sizeof(Paths));
    token = strtok(path, sep);
    strcpy(&paths.path[paths.size++][0], token);
    while( token != NULL ) {
        // printf( "%s\n", token );
        token = strtok(NULL, sep);
        if (token != NULL)
            strcpy(&paths.path[paths.size++][0], token);
    }
    return paths;
}


void str_to_upper(char *p){
    for (int i = 0; i < strlen(p); i++)
        p[i] = toupper(p[i]);
}
