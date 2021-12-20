
#include "filesystem.h"

typedef struct disc_info disc_info;
typedef struct descriptor descriptor;

int create_disc(char *name, int size)
{
    FILE *disc;
    char buffer[1];
    disc_info info;
    int sizeBytes = size*BLOCK;

    disc = fopen(name, "r"); // check whether such disc exists
    if(size < 3)
    {
        printf("Capacity to small. Cannot create new disc.\n");
        return -1;
    }

    if(disc != NULL)
    {
        printf("Disc %s already exists. Give another name.\n", name);
        fclose(disc);
        return -1;
    }

	// if name is new - create new disc file and open in binary mode
    disc = fopen(name, "wb+");

    fseek(disc, sizeBytes-1, SEEK_SET);
    buffer[0] = '0';
    fwrite(buffer, sizeof(char), 1, disc);

    fseek(disc, 0, SEEK_SET);
    info.no_of_descriptor_blocks = 2;
    info.size = size;
    info.free = size-2;
    info.f_count = 0;
    fwrite(&info, sizeof(disc_info), 1, disc);

    fclose(disc);
    return 0;
}

int delete_disc(char* name)
{
    if(unlink(name) != 0) //function unlink defined in unistd.h (POSIX)
    {
        printf("Cannot remove disc %s.\n", name);
        return -1;
    }
    printf("Disc %s was removed.\n", name);
    return 0;
}

int upload(char* disc_name, char* file_name)
{
    FILE *disc = fopen(disc_name, "r+b");
    if(disc == NULL)
    {
        printf("Disc '%s' doesn't exist.\n", disc_name);
        fclose(disc);
        return -1;
    }
    
    FILE *file = fopen(file_name, "r");
    if(file == NULL)
    {
        printf("File '%s' doesn't exist.\n", file_name);
        fclose(disc);
        fclose(file);
        return -1;
    }

    if(MAX_LENGTH < strlen(file_name))
    {
        printf("Name of file '%s' too long. Maximum size is 19 characters.\n", file_name);
        fclose(disc);
        fclose(file);
        return -1;
    }

    fseek(file, 0, SEEK_END);
    long int size_in_bytes = ftell(file);
    // number of blocks necessary to write this file
    long int size = (size_in_bytes + BLOCK -1) / BLOCK;
    // internal fragmentation
    int free_bytes = size * BLOCK - size_in_bytes;
    char *buffer = malloc(sizeof(char)* size_in_bytes);
    // reaad dics descriptor
    disc_info info;
    fread(&info, sizeof(disc_info), 1, disc);

    // check if there is enough space for this file
    if (size > info.free)
    {
        printf("There is no enough space on disc '%s' for file %s.\n", disc_name, file_name);
        fclose(disc);
        fclose(file);
        free(buffer);
        return -1;
    }

    descriptor desc;

    // check if file with this file_name already exists on this disc
    for (int i=0; i<info.f_count; i++)
    {
        fread(&desc, sizeof(descriptor), 1, disc);
        if (strcmp(desc.name, file_name) == 0)
        {
            printf("File '%s' already exists on disc '%s'.\n", file_name, disc_name);
            fclose(disc);
            fclose(file);
            free(buffer);
            return -1;
        }
    }
    
    if (info.f_count == 0)
    {
        fseek(file, 0, SEEK_SET);
        fread(buffer, sizeof(char), size_in_bytes, file);

        fseek(disc, info.no_of_descriptor_blocks * BLOCK, SEEK_SET);
        fwrite(buffer, sizeof(char), size_in_bytes, disc);

        //save information for the first file
        desc.adress = info.no_of_descriptor_blocks;
        desc.size = size;
        desc.free_bytes = free_bytes;
        strcpy(desc.name, file_name);
        info.f_count = 1;
        info.free -= desc.size;

        fseek(disc, 0, SEEK_SET);
        fwrite(&info, sizeof(disc_info), 1, disc); // update information in descriptors' segment
        fwrite(&desc, sizeof(descriptor), 1, disc);
        fclose(disc);
        fclose(file);
        free(buffer);
        return 0;
    }

    // check if there is enough space for new file descriptor
    if (info.no_of_descriptor_blocks * BLOCK - info.f_count * sizeof(info) -
        sizeof(disc_info) - sizeof(info) < 0)
    {
        if (add_descriptor_block(disc) == -1)
        {
            printf("Lack of free space on disc '%s' for file descriptor.\n", disc_name);
            fclose(disc);
            fclose(file);
            free(buffer);
            return -1;
        }
        else
        {
            fseek(disc, 0, SEEK_SET);
            info.free--;
            fread(&info, sizeof(disc_info), 1, disc);
        }
    }

    long int position;
    fseek(disc, sizeof(disc_info) + (info.f_count - 1)*sizeof(descriptor), SEEK_SET);
    fread(&desc, sizeof(descriptor), 1, disc); // load descriptor of last file
    if ((info.size - (desc.adress + desc.size)) > size) //check whether is enough continuous space for file
    {                                                   //if not - it's necessary to shift data
        position = ftell(disc);
        fseek(file, 0, SEEK_SET);
        fread(buffer, sizeof(char), size_in_bytes, file);
        fseek(disc, (desc.adress + desc.size) * BLOCK, SEEK_SET);
        fwrite(buffer, sizeof(char), size_in_bytes, disc);
        fseek(disc, position, SEEK_SET);
        desc.adress = desc.adress + desc.size; //save information for file
        desc.size = size;
        strcpy(desc.name, file_name);
        desc.free_bytes = free_bytes;
        fwrite(&desc, sizeof(descriptor), 1, disc); //update information for file in descriptors' segment
        info.f_count++;
        info.free-=desc.size;
        fseek(disc, 0, SEEK_SET);
        fwrite(&info, sizeof(disc_info), 1, disc);
        fclose(disc);
        fclose(file);
        free(buffer);
        return 0;
    }

    if (reallocate(disc) == 0) // earlier checked whether is enough space - now try to reallocate
    {
        fseek(disc, 0, SEEK_SET);
        fread(&info, sizeof(disc_info), 1, disc);
        fseek(disc, sizeof(disc_info) + (info.f_count - 1) * sizeof(descriptor), SEEK_SET);
        fread(&desc, sizeof(descriptor), 1, disc); //read descriptor of last file
        position = ftell(disc);
        fseek(file, 0, SEEK_SET);
        fread(buffer, sizeof(char), size_in_bytes, file);
        fseek(disc, (desc.adress + desc.size) * BLOCK, SEEK_SET);
        fwrite(buffer, sizeof(char), size_in_bytes, disc);
        fseek(disc, position, SEEK_SET);
        desc.adress = desc.adress + desc.size; //save information for file
        desc.size = size;
        strcpy(desc.name, file_name);
        desc.free_bytes = free_bytes;
        fwrite(&desc, sizeof(descriptor), 1, disc); //update information for file in descriptors' segment
        info.f_count++;
        info.free-=desc.size;
        fseek(disc, 0, SEEK_SET);
        fwrite(&info, sizeof(disc_info), 1, disc);
        fclose(disc);
        fclose(file);
        free(buffer);
        return 0;
    }

    fclose(disc);
    fclose(file);
    free(buffer);
    printf("Error occurs. Try again.\n");
    return -1;
}

int download(char* disc_name, char* file_name)
{
    FILE *disc, *file;
    disc_info info;
    descriptor current;
    char* buffer;
    int i;
    disc = fopen(disc_name, "r+b");

    file = fopen(file_name, "r");        //check whether such file exists in local directory
    if(file != NULL)
    {
        printf("File %s already exists in local directory.\n", file_name);
        fclose(disc);
        return -1;
    }
    if(disc == NULL)
    {
        printf("Disc %s doesn't exist.\n", disc_name);
        return -1;
    }
    fread(&info, sizeof(disc_info), 1, disc);
    for(i = 0; i < info.f_count; i++)
    {
        fread(&current, sizeof(descriptor), 1, disc);
        if( strcmp(current.name, file_name) == 0)
        {
            buffer = malloc (sizeof(char) * current.size * BLOCK - current.free_bytes);
            fseek(disc, current.adress *BLOCK, SEEK_SET);
            fread(buffer, sizeof(char), current.size *BLOCK - current.free_bytes, disc);
            file = fopen(file_name, "wb");
            fwrite(buffer, sizeof(char), current.size *BLOCK - current.free_bytes, file);
            fclose(file);
            fclose(disc);
            return 0;
        }
    }
    printf("File %s doesn't exist on disc %s.\n", file_name, disc_name);
    fclose(disc);
    return 1;
}

int delete_file(char* disc_name, char* file_name)
{
    FILE* disc;
    disc_info info;
    descriptor current;
    int i;
    disc = fopen(disc_name, "r+b");
    if(disc == NULL)
    {
        printf("Disc %s doesn't exist.\n", disc_name);
        return -1;
    }
    fread(&info, sizeof(disc_info), 1, disc);
    fread(&current, sizeof(descriptor), 1, disc);
    if( strcmp( current.name, file_name) == 0)        //check first file
    {
        shift_right(disc, sizeof(descriptor), sizeof(disc_info) + sizeof(descriptor), (info.f_count - 1) * sizeof(descriptor));
        info.f_count--;
        info.free+=current.size;
        fseek(disc, 0, SEEK_SET);
        fwrite(&info, sizeof(disc_info), 1, disc);
        fclose(disc);
        return 0;
    }
    for(i = 1; i < info.f_count; i++)
    {
        fread(&current, sizeof(descriptor), 1, disc);
        if( strcmp( current.name, file_name) == 0)
        {
            shift_right(disc, sizeof(descriptor), sizeof(disc_info) + (i+1) * sizeof(descriptor), (info.f_count - i - 1) * sizeof(descriptor));
            info.f_count--;
            info.free+=current.size;
            fseek(disc, 0, SEEK_SET);
            fwrite(&info, sizeof(disc_info), 1, disc);
            fclose(disc);
            return 0;
        }
    }
    printf("File %s doesn't exist on disc %s.\n", file_name, disc_name);
    fclose(disc);
    return -1;
}

int ls(char* disc_name)
{
    FILE *disc;
    int i;
    disc_info info;
    descriptor current;
    disc = fopen(disc_name, "r+b");
    if(disc == NULL)
    {
        printf("Disc %s doesn't exist.\n", disc_name);
        return -1;
    }
    fread(&info, sizeof(disc_info), 1, disc);
    for(i = 0; i < info.f_count; i++)
    {
        fread(&current, sizeof(descriptor), 1, disc);
        printf("%d. file's name:\t%s\t", (i+1), current.name);
        printf("Size in blocks:\t%d\tSize in bytes:\t%d\n", current.size, (current.size)*BLOCK-current.free_bytes);
    }
    printf("\n");
    fclose(disc);
    return 0;
}

int info(char* disc_name)
{
    FILE* disc;
    disc_info info;
    disc = fopen(disc_name, "r+b");
    if (disc == NULL)
    {
        printf("Disc %s doesn't exist.\n", disc_name);
        return -1;
    }
    fread(&info, sizeof(disc_info), 1, disc);
    printf(
        "Number of descriptor blocks:\t%d\n"
        "Number of files saved on disc:\t%d\n"
        "Free space on disc in blocks:\t%d\n"
        "Free space on disc in bytes:\t%d\n\n",
        info.no_of_descriptor_blocks, info.f_count, info.free, info.free*BLOCK
    );
    fclose(disc);
    return 0;
}

void help()
{
    printf(
    "First argument is always name of disc. All available commands below:\n"
    "new \t\t<arg1> <arg2>\t\tCreate disc <arg1> of size <arg2>\n"
    "delete \t\t<arg1>\t\t\tDelete disc <arg1> \n"
    "upload \t\t<arg1> <arg2>\t\tCopy file <arg2> to disc <arg1>\n"
    "download \t<arg1> <arg2>\t\tCopy file <arg2> from disc <arg2> to local user's folder\n"
    "remove \t\t<arg1> <arg2>\t\tDelete file <arg2> from disc <arg1>\n"
    "ls \t\t<arg1>\t\t\tShow list of files saved on disc <arg1>\n"
    "info \t\t<arg1>\t\t\tCheck disc <arg1> condition\n"
    "help \t\t\t\t\tDisplay help\n"
    "quit \t\t\t\t\tFinish work with program\n\n"
    );
    return;
}


int reallocate(FILE* f)
{
    int nochange = 1;        //a priori let's assume that no changes was made
    int shift;
    int position;
    fseek(f, 0, SEEK_SET);
    disc_info info;
    descriptor current, prev;

    fread(&info, sizeof(disc_info), 1, f);
    fread(&current, sizeof(descriptor), 1, f);
    shift = current.adress - info.no_of_descriptor_blocks;        //shift first file
    if(shift != 0)
    {
        shift_right(f, shift * BLOCK, current.adress * BLOCK, current.size * BLOCK);
        current.adress = current.adress - shift;
        fseek(f, sizeof(disc_info), SEEK_SET);
        fwrite(&current, sizeof(descriptor), 1, f);
        nochange =0;
    }
    prev = current;
    for(int i = 1; i < info.f_count; i++)
    {
        fread(&current, sizeof(descriptor), 1, f);
        shift = current.adress - (prev.adress + prev.size);
        if(shift != 0)
        {
            position = ftell(f);            //remember position in descriptor segmnent before shifting
            shift_right(f, shift * BLOCK, current.adress * BLOCK, current.size * BLOCK);
            fseek(f, position, SEEK_SET);            //restore position after shifting
            current.adress = current.adress - shift;
            fwrite(&current, sizeof(descriptor), 1, f);
            nochange = 0;
        }
        prev = current;
    }
    return nochange;
}

int add_descriptor_block(FILE* f) //add new descriptor block if necessary
{
    disc_info info;
    descriptor desc;

    fseek(f, 0, SEEK_SET);
    fread(&info, sizeof(disc_info), 1, f);
    fseek(f, (info.f_count - 1) * sizeof(descriptor), SEEK_CUR);
    fread(&desc, sizeof(descriptor), 1, f); //read descriptor of last file on disc
    if (desc.adress + desc.size == info.size) //no enough place for new descriptor block - try reallocate data
    {
        if (reallocate(f) == 1) { //if no change was made - we don't have space for new descriptor
            return -1;
        }
    }
    shift_left(f, BLOCK, info.no_of_descriptor_blocks *BLOCK, (info.size - info.no_of_descriptor_blocks -1) * BLOCK);
    info.no_of_descriptor_blocks++;
    return 0;
}

int shift(FILE* f, int shift_, int offset, int size) // shift - distance in bytes to shift;
{                                                    // offset - position of bytes to shift
    if(size == 0) return 1;                          // size - number of bytes to shift
    if(shift_ == 0) return 1;
    char *buffer;
    buffer = malloc(sizeof(char)*size);
    fseek(f, offset, SEEK_SET);
    fread(buffer, sizeof(char), size, f);
    fseek(f, offset - shift_, SEEK_SET);
    fwrite(buffer, sizeof(char), size, f);
    free(buffer);
    return 0;
}

int shift_right(FILE* f, int shift_, int offset, int size)
{
    return shift(f, shift_, offset, size);
}

int shift_left(FILE* f, int shift_, int offset, int size)
{
    return shift(f, -shift_, offset, size);
}
