#ifndef STX_H
#define STX_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <wchar.h>

typedef struct
{
    int32_t unknown;
    int32_t string_count;
    wchar_t **strings;
} STXFile;

STXFile *stx_load(const char *filename);
void stx_save(STXFile *file, const char *filename);
void stx_free(STXFile *file);

#ifdef __cplusplus
}
#endif

#endif