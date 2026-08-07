#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <wchar.h>
#include <windows.h>

#define EXECUTABLE_NAME "eztt"

#define STX_FILE_MAGIC "STXT"
#define STX_LANG_MAGIC "JPLL"

// Referenced:
// https://github.com/CaptainSwag101/DRV3-Sharp

int main(int argc, char *argv[])
{
    if(argc <= 1)
    {
        printf("Please provide a file!\nUsage: %s <file_name>\n", EXECUTABLE_NAME);
        return 1;
    }

    FILE *file = fopen(argv[1], "rb");

    // First, let's just try to read the header data

    char file_magic[5];
    char lang_magic[5];

    if(!fgets(file_magic, sizeof(file_magic), file))
    {
        printf("Couldn't read file magic (for whatever reason).\n");
        return 1;
    }

    if(strcmp(file_magic, STX_FILE_MAGIC) != 0)
    {
        printf("Invalid file magic \"%s\". Wanted \"%s\"\n", file_magic, STX_FILE_MAGIC);
        return 1;
    }

    if(!fgets(lang_magic, sizeof(lang_magic), file))
    {
        printf("Couldn't read lang magic (for whatever reason).\n");
        return 1;
    }

    if(strcmp(lang_magic, STX_LANG_MAGIC) != 0)
    {
        printf("Invalid lang magic \"%s\". Wanted \"%s\"\n", lang_magic, STX_LANG_MAGIC);
        return 1;
    }

    int32_t table_count;
    int ret = fread(&table_count, 4, 1, file);
    if(ret != 1)
    {
        fprintf(stderr, "fread() failed: %zu\n", ret);
        exit(EXIT_FAILURE);
    }

    if(table_count > 1)
    {
        printf("Table counts above 1 are unsupported!\n");
        return 1;
    }

    uint32_t table_offset;
    ret = fread(&table_offset, 4, 1, file);
    if(ret != 1)
    {
        fprintf(stderr, "fread() failed: %zu\n", ret);
        exit(EXIT_FAILURE);
    }
    
    printf("HEADER INFO:\n================\n");
    printf("FILE_MAGIC: %s\n", file_magic);
    printf("LANG_MAGIC: %s\n", lang_magic);
    printf("TABLE_COUNT: %d\n", table_count);
    printf("TABLE_OFFSET: %zu\n", table_offset);

    // Read table info

    int32_t unknown;
    int32_t string_count;
    ret = fread(&unknown, 4, 1, file);
    ret = fread(&string_count, 4, 1, file);
    if(ret != 1)
    {
        fprintf(stderr, "fread() failed: %zu\n", ret);
        exit(EXIT_FAILURE);
    }

    printf("-\nString count: %d\n", string_count);

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

    fclose(file);
    for(int i = 0; i < string_count; i++)
        wprintf(L"[%d] %ls\n", i, str_list[i]);

    while(1)
    {
        printf("(0) View Dialogue\n(1) Save STX file\nSelection (q to quit): ");
        uint32_t input;
        if(scanf("%d", &input) != 1 && getchar() == 'q')
            break;
        
        while (getchar() != '\n');

        if(input < 0 || input > 1)
        {
            printf("Invalid selection (out of bounds)!\n");
            continue;
        }

        if(input == 1)
        {
            // Save STX file...

            // TODO: Get file name

            const char *filename = "test.stx";

            FILE *save = fopen(filename, "wb");

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

            // write string data & corresponding ID/offset pair
            for(uint32_t i = 0; i < string_count; i++)
            {
                str_offsets[i] = (uint32_t)ftell(save);
                fwrite(str_list[i], sizeof(wchar_t), wcslen(str_list[i]) + 1, save);

                wprintf(L"[%zu, %zu] %ls\n", i, str_offsets[i], str_list[i]);
            }

            fseek(save, table_offset, SEEK_SET);
            for(uint32_t i = 0; i < string_count; i++)
            {
                fwrite(&i, sizeof(uint32_t), 1, save);
                fwrite(&str_offsets[i], sizeof(uint32_t), 1, save);
            }

            fclose(save);

            continue;
        }
        
        printf("Select ID (q to quit): ");

        if(scanf("%d", &input) != 1 && getchar() == 'q')
            break;
        
        while (getchar() != '\n');

        if(input < 0 || input >= string_count)
        {
            printf("Invalid ID (out of bounds)!\n");
            continue;
        }

view_dialogue:
        wprintf(L"[%d] %ls\n", input, str_list[input]);
        printf("\n(0) Back\n(1) Edit Text\nSelection (q to quit): ");

        uint32_t edit;
        if(scanf("%d", &edit) != 1)
        {
            if(getchar() == 'q')
                break;
            else if(edit < 0 || edit > 1)
            {
                printf("Invalid selection!\n");
                goto view_dialogue;
            }
        }

        while (getchar() != '\n');

        if(edit)
        {
            wprintf(L"Enter new text: ");

            wchar_t buffer[1024];
            fgetws(buffer, 1024, stdin);
            buffer[wcscspn(buffer, L"\n")] = L'\0';

            free(str_list[input]);
            str_list[input] = wcsdup(buffer);

            goto view_dialogue;
        }
    }

    for(int i = 0; i < string_count; i++)
        free(str_list[i]);

    free(str_list);
    free(str_offsets);

    return 0;
}