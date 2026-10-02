#ifndef _INCLUDE_KANACO_H
#define _INCLUDE_KANACO_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CNV_ASIS 0
#define CNV_LOWER_R 1
#define CNV_UPPER_R 2
#define CNV_LOWER_N 4
#define CNV_UPPER_N 8
#define CNV_LOWER_A 16
#define CNV_UPPER_A 32
#define CNV_LOWER_S 64
#define CNV_UPPER_S 128
#define CNV_LOWER_K 256
#define CNV_UPPER_K 512
#define CNV_LOWER_H 1024
#define CNV_UPPER_H 2048
#define CNV_LOWER_C 4096
#define CNV_UPPER_C 8192

/*
 * Converts the UTF-8 string str (str_len bytes) according to mode (mode_len
 * bytes, e.g. "Kas"). The result is a newly malloc'ed, NUL-terminated
 * buffer the caller must free(). Its length in bytes is stored in *out_len
 * when out_len is not NULL; use it rather than strlen() when the input may
 * contain NUL bytes. An empty mode yields an empty string.
 * Returns NULL only when memory cannot be allocated.
 */
char *kanaco_convert(const char *str, size_t str_len, const char *mode,
                     size_t mode_len, size_t *out_len);

/* Same as kanaco_convert() without the output length. Kept for
 * compatibility. */
char *convert(const char *str, int str_len, const char *mode, int mode_len);

#ifdef __cplusplus
}
#endif

#endif
