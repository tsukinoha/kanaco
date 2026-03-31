#ifndef _INCLUDE_KANACO_H
#define _INCLUDE_KANACO_H

#include <stdbool.h>
#include <stdint.h>

#define CNV_ASIS 0
#define CNV_LOWER_R 2<<0
#define CNV_UPPER_R 2<<1
#define CNV_LOWER_N 2<<2
#define CNV_UPPER_N 2<<3
#define CNV_LOWER_A 2<<4
#define CNV_UPPER_A 2<<5
#define CNV_LOWER_S 2<<6
#define CNV_UPPER_S 2<<7
#define CNV_LOWER_K 2<<8
#define CNV_UPPER_K 2<<9
#define CNV_LOWER_H 2<<10
#define CNV_UPPER_H 2<<11
#define CNV_LOWER_C 2<<12
#define CNV_UPPER_C 2<<13

typedef struct _character
{
  char val[8];
  uint8_t len;
  uint16_t conv; // CNV_LOWER_* or CNV_UPPER_*
  uint8_t cval[8];  // converted value
  uint8_t clen;  // converted value length;
} character;

typedef void (*filter)(character *);

bool is_1byte(const char *, int);
bool is_2bytes(const char *, int);
bool is_3bytes(const char *, int);
bool is_4bytes(const char *, int);
bool is_voiced(const char *, int);
bool is_semi_voiced(const char *, int);

void lower_r(character *);
void upper_r(character *);
void lower_n(character *);
void upper_n(character *);
void lower_a(character *);
void upper_a(character *);
void lower_s(character *);
void upper_s(character *);
void lower_k(character *);
void upper_k(character *);
void lower_h(character *);
void upper_h(character *);
void lower_c(character *);
void upper_c(character *);
void asis(character *);

filter *create_filters(const char *, int);
void init_character(character *);
void conv(character *, filter *);
void extract(character *c, const char *s, int len);

extern char *convert(const char *, int, const char *, int);

#endif
