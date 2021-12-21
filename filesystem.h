
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#define BLOCK 256
#define MAX_LENGTH 20

struct disc_info
{
    int no_of_descriptor_blocks;
    int size;
    int free;
    int f_count;
};

struct descriptor
{
    int adress;
    int size;
    int free_bytes;
    char name[MAX_LENGTH];
};

int create_disc(char *disc_name, int size);
int delete_disc(char *disc_name);

int upload(char* disc_name, char* file_name);
int add_new_file(FILE* disc, FILE* file, char *buffer, long int size_in_bytes,
                 long int size, int free_bytes, char* file_name, struct disc_info info, struct descriptor desc);
int download(char* disc_name, char* file_name);

int delete_file(char* disc_name, char* file_name);

int ls(char *disc_name);
int info(char* disc_name);
void help();

int reallocate(FILE* f);
int add_descriptor_block(FILE* f);
int shift(FILE* f, int shift, int offset, int size);
int shift_right(FILE* f, int shift, int offset, int size);
int shift_left(FILE* f, int shift, int offset, int size);
