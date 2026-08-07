#include "stx.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define STX_FILE_MAGIC "STXT"
#define STX_LANG_MAGIC "JPLL"

STXFile *stx_load(const char *filename)
{
    FILE *file = fopen(filename, "rb");

    char file_magic[5];
    char lang_magic[5];

    if(!fgets(file_magic, sizeof(file_magic), file))
    {
        printf("Couldn't read file magic (for whatever reason).\n");
        fclose(file);
        return NULL;
    }

    if(strcmp(file_magic, STX_FILE_MAGIC) != 0)
    {
        printf("Invalid file magic \"%s\". Wanted \"%s\"\n", file_magic, STX_FILE_MAGIC);
        fclose(file);
        return NULL;
    }

    if(!fgets(lang_magic, sizeof(lang_magic), file))
    {
        printf("Couldn't read lang magic (for whatever reason).\n");
        fclose(file);
        return NULL;
    }

    if(strcmp(lang_magic, STX_LANG_MAGIC) != 0)
    {
        printf("Invalid lang magic \"%s\". Wanted \"%s\"\n", lang_magic, STX_LANG_MAGIC);
        fclose(file);
        return NULL;
    }

    int32_t table_count;
    int ret = fread(&table_count, 4, 1, file);
    if(ret != 1)
    {
        printf("fread() failed: %zu\n", ret);
        fclose(file);
        return NULL;
    }

    if(table_count > 1)
    {
        printf("Table counts above 1 are unsupported!\n");
        fclose(file);
        return NULL;
    }

    uint32_t table_offset;
    ret = fread(&table_offset, 4, 1, file);
    if(ret != 1)
    {
        printf("fread() failed: %zu\n", ret);
        fclose(file);
        return NULL;
    }

    int32_t unknown;
    int32_t string_count;
    ret = fread(&unknown, 4, 1, file);
    if(ret != 1)
    {
        printf("fread() failed: %zu\n", ret);
        fclose(file);
        return NULL;
    }
    ret = fread(&string_count, 4, 1, file);
    if(ret != 1)
    {
        printf("fread() failed: %zu\n", ret);
        fclose(file);
        return NULL;
    }

    // Read table data
    fseek(file, table_offset, SEEK_SET);

    uint32_t *str_offsets = malloc(sizeof(uint32_t) * string_count);

    for(int i = 0; i < string_count; i++)
    {
        uint32_t string_id;
        uint32_t string_offset;

        fread(&string_id, 4, 1, file);
        fread(&string_offset, 4, 1, file);

        if(string_id < string_count)
            str_offsets[string_id] = string_offset;
    }

    wchar_t **str_list = malloc(sizeof(wchar_t *) * string_count);
    
    for(int i = 0; i < string_count; i++)
    {
        fseek(file, str_offsets[i], SEEK_SET);

        wchar_t str[1024];
        int c = 0;
        while(fread(&str[c++], sizeof(wchar_t), 1, file))
        {
            if(str[c-1] == 0)
                break;
        }

        str_list[i] = wcsdup(str);
    }

    free(str_offsets);
    fclose(file);

    STXFile *stx = malloc(sizeof(STXFile));
    stx->unknown = unknown;
    stx->string_count = string_count;
    stx->strings = str_list;

    return stx;
}

void stx_save(STXFile *file, const char *filename)
{
    FILE *save = fopen(filename, "wb");

    uint32_t table_count = 1;
    uint32_t table_offset = 32;
    uint32_t unknown = file->unknown;
    int32_t string_count = file->string_count;

    // write header data
    fwrite(STX_FILE_MAGIC,  1, 4, save); 
    fwrite(STX_LANG_MAGIC,  1, 4, save); 
    fwrite(&table_count,    sizeof(table_count),    1, save);
    fwrite(&table_offset,   sizeof(table_offset),   1, save);

    fwrite(&unknown,        sizeof(unknown), 1, save);
    fwrite(&string_count,   sizeof(string_count), 1, save);

    // Pad to 16 byte
    uint32_t zero = 0;
    fwrite(&zero, 4, 1, save);
    fwrite(&zero, 4, 1, save);

    // write temporary padding for string IDs + offsets
    for(int i = 0; i < string_count; i++) // 8 bytes per string entry
    {
        fwrite(&zero, sizeof(uint32_t), 1, save);
        fwrite(&zero, sizeof(uint32_t), 1, save);
    }

    uint32_t *str_offsets = malloc(sizeof(uint32_t) * string_count);

    // write string data & corresponding ID/offset pair
    for(uint32_t i = 0; i < string_count; i++)
    {
        str_offsets[i] = (uint32_t)ftell(save);
        fwrite(file->strings[i], sizeof(wchar_t), wcslen(file->strings[i]) + 1, save);

        wprintf(L"[%zu, %zu] %ls\n", i, str_offsets[i], file->strings[i]);
    }

    fseek(save, table_offset, SEEK_SET);
    for(uint32_t i = 0; i < string_count; i++)
    {
        fwrite(&i, sizeof(uint32_t), 1, save);
        fwrite(&str_offsets[i], sizeof(uint32_t), 1, save);
    }

    free(str_offsets);
    fclose(save);
}

void stx_free(STXFile *file)
{
    for(int i = 0; i < file->string_count; i++)
        free(file->strings[i]);

    free(file);
}