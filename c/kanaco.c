#include "kanaco.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define BUF_CHARS 6 /* the longest converted character, e.g. ｶﾞ */
#define MAX_FILTERS 14

/*
 * A parsed mode: the filters in the order they were given (a later filter
 * wins when several apply) and their union mask.
 */
typedef struct {
  int filters[MAX_FILTERS];
  int n;
  int mask;
} converter;

static int put1(uint8_t *out, uint8_t b0) {
  out[0] = b0;
  return 1;
}

static int put3(uint8_t *out, uint8_t b0, uint8_t b1, uint8_t b2) {
  out[0] = b0;
  out[1] = b1;
  out[2] = b2;
  return 3;
}

static int put6(uint8_t *out, uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3,
                uint8_t b4, uint8_t b5) {
  out[0] = b0;
  out[1] = b1;
  out[2] = b2;
  out[3] = b3;
  out[4] = b4;
  out[5] = b5;
  return 6;
}

/* Filters applicable to each 1-byte character. */
static int ascii_flags(uint8_t c) {
  if (c == 0x20) { /* Space */
    return CNV_UPPER_S;
  } else if (c >= 0x30 && c <= 0x39) { /* 0 - 9 */
    return CNV_UPPER_A | CNV_UPPER_N;
  } else if (c >= 0x41 && c <= 0x5a) { /* A - Z */
    return CNV_UPPER_A | CNV_UPPER_R;
  } else if (c >= 0x61 && c <= 0x7a) { /* a - z */
    return CNV_UPPER_A | CNV_UPPER_R;
  } else if (c >= 0x21 && c <= 0x7d && c != 0x22 && c != 0x27 && c != 0x5c) {
    return CNV_UPPER_A;
  }
  return CNV_ASIS;
}

static int ascii_table[0x80];
static volatile int ascii_table_ready = 0;

static void init_ascii_table(void) {
  /* Idempotent: concurrent first calls write identical values. */
  for (int c = 0; c < 0x80; c++) {
    ascii_table[c] = ascii_flags((uint8_t)c);
  }
  ascii_table_ready = 1;
}

static int is_voiced(const uint8_t *b, size_t len) {
  if (len < 6) {
    return 0;
  }
  if (b[3] == 0xef && b[4] == 0xbe && b[5] == 0x9e) {
    if (b[0] == 0xef && b[1] == 0xbd && b[2] > 0xb5 && b[2] < 0xc0) { /* ｶ - ｿ */
      return 1;
    } else if (b[0] == 0xef && b[1] == 0xbe && b[2] > 0x79 && b[2] < 0x85) { /* ﾀ - ﾄ */
      return 1;
    } else if (b[0] == 0xef && b[1] == 0xbe && b[2] > 0x89 && b[2] < 0x8f) { /* ﾊ - ﾎ */
      return 1;
    } else if (b[0] == 0xef && b[1] == 0xbd && b[2] == 0xb3) { /* ｳ */
      return 1;
    }
  }
  return 0;
}

static int is_semi_voiced(const uint8_t *b, size_t len) {
  if (len < 6) {
    return 0;
  }
  if (b[3] == 0xef && b[4] == 0xbe && b[5] == 0x9f) {
    if (b[0] == 0xef && b[1] == 0xbe && b[2] > 0x89 && b[2] < 0x8f) { /* ﾊ - ﾎ */
      return 1;
    }
  }
  return 0;
}

/*
 * Returns the byte length of the multi-byte character at the head of s and
 * stores the filters applicable to it in *flags. s[0] must be >= 0x80.
 */
static size_t extract(const uint8_t *s, size_t len, int *flags) {
  *flags = CNV_ASIS;
  if (len < 3 || (s[0] & 0xe0) != 0xe0 || !(s[1] & 0x80) || !(s[2] & 0x80)) {
    if (len >= 2 && (s[0] & 0xc2) == 0xc2 && (s[1] & 0x80)) {
      return 2;
    }
    return 1;
  }
  uint8_t c0 = s[0], c1 = s[1], c2 = s[2];
  if (c0 == 0xef) {
    if (c1 == 0xbc) {
      if (c2 >= 0x90 && c2 <= 0x99) { /* ０ - ９ */
        *flags = CNV_LOWER_A | CNV_LOWER_N;
      } else if (c2 >= 0xa1 && c2 <= 0xba) { /* Ａ - Ｚ */
        *flags = CNV_LOWER_A | CNV_LOWER_R;
      } else if (c2 != 0x82 && c2 != 0x87 && c2 != 0xbc) { /* except ＂ ＇ ＼ */
        *flags = CNV_LOWER_A;
      }
    } else if (c1 == 0xbd) {
      if (c2 >= 0x81 && c2 <= 0x9a) { /* ａ - ｚ */
        *flags = CNV_LOWER_A | CNV_LOWER_R;
      } else if (c2 >= 0x80 && c2 <= 0x9d) { /* ｀ - ｝ */
        *flags = CNV_LOWER_A;
      } else if (c2 >= 0xa1 && c2 <= 0xbf) { /* ｡ ｢ ｣ ､ ･ ｦ - ｿ */
        *flags = CNV_UPPER_H | CNV_UPPER_K;
        if (is_voiced(s, len)) {
          return 6;
        }
      }
    } else if (c1 == 0xbe) {
      if (c2 >= 0x80 && c2 <= 0x84) { /* ﾀ - ﾄ */
        *flags = CNV_UPPER_H | CNV_UPPER_K;
        if (is_voiced(s, len)) {
          return 6;
        }
      } else if (c2 >= 0x8a && c2 <= 0x8e) { /* ﾊ - ﾎ */
        *flags = CNV_UPPER_H | CNV_UPPER_K;
        if (is_voiced(s, len) || is_semi_voiced(s, len)) {
          return 6;
        }
      } else if (c2 >= 0x85 && c2 <= 0x9f) { /* ﾅ - ﾝﾞﾟ */
        *flags = CNV_UPPER_H | CNV_UPPER_K;
      }
    }
  } else if (c0 == 0xe3) {
    if (c1 == 0x80) {
      if (c2 == 0x80) { /* Space */
        *flags = CNV_LOWER_S;
      } else if (c2 >= 0x81 && c2 <= 0x82) { /* 、。 */
        *flags = CNV_LOWER_H | CNV_LOWER_K;
      }
    } else if (c1 == 0x81) {
      if (c2 >= 0x81 && c2 <= 0xbf) { /* ぁ - み */
        *flags = CNV_UPPER_C | CNV_LOWER_H;
      }
    } else if (c1 == 0x82) {
      if (c2 >= 0x80 && c2 <= 0x93) { /* む - ん */
        *flags = CNV_UPPER_C | CNV_LOWER_H;
      } else if (c2 >= 0x9b && c2 <= 0x9c) { /* ゛゜ */
        *flags = CNV_LOWER_H | CNV_LOWER_K;
      } else if (c2 >= 0x9d && c2 <= 0x9e) { /* ゝゞ */
        *flags = CNV_UPPER_C;
      } else if (c2 >= 0xa1 && c2 <= 0xbf) { /* ァ - タ */
        *flags = CNV_LOWER_C | CNV_LOWER_K;
      }
    } else if (c1 == 0x83) {
      if (c2 >= 0x80 && c2 <= 0xb3) { /* チ - ン */
        *flags = CNV_LOWER_C | CNV_LOWER_K;
      } else if (c2 == 0xb4) { /* ヴ */
        *flags = CNV_LOWER_K;
      } else if (c2 >= 0xbb && c2 <= 0xbc) { /* ・ー */
        *flags = CNV_LOWER_H | CNV_LOWER_K;
      } else if (c2 >= 0xbd && c2 <= 0xbe) { /* ヽ ヾ */
        *flags = CNV_LOWER_C;
      }
    }
  }
  return 3;
}

static int lower_r(const uint8_t *v, size_t n, uint8_t *out) {
  (void)n;
  // Ａ-Ｚ -> A-Z
  if (v[2] >= 0xa1 && v[2] <= 0xba) {
    return put1(out, v[2]-0x60);
  }
  // ａ-ｚ -> a-z
  if (v[2] >= 0x81 && v[2] <= 0x9a) {
    return put1(out, v[2]-0x20);
  }
  return 0;
}

static int upper_r(const uint8_t *v, size_t n, uint8_t *out) {
  (void)n;
  // A-Z -> Ａ-Ｚ
  if (v[0] >= 0x41 && v[0] <= 0x5a) {
    return put3(out, 0xef, 0xbc, v[0]+0x60);
  }
  // a-z -> ａ-ｚ
  if (v[0] >= 0x61 && v[0] <= 0x7a) {
    return put3(out, 0xef, 0xbd, v[0]+0x20);
  }
  return 0;
}

static int lower_n(const uint8_t *v, size_t n, uint8_t *out) {
  (void)n;
  return put1(out, v[2]-0x60);
}

static int upper_n(const uint8_t *v, size_t n, uint8_t *out) {
  (void)n;
  return put3(out, 0xef, 0xbc, v[0]+0x60);
}

static int lower_a(const uint8_t *v, size_t n, uint8_t *out) {
  (void)n;
  if (v[1] == 0xbc && v[2] >= 0x81 && v[2] <= 0xbf) {
    return put1(out, v[2]-0x60);
  }
  if (v[1] == 0xbd && v[2] >= 0x80 && v[2] <= 0x9d) {
    return put1(out, v[2]-0x20);
  }
  return 0;
}

static int upper_a(const uint8_t *v, size_t n, uint8_t *out) {
  (void)n;
  if (v[0] >= 0x21 && v[0] <= 0x5f) {
    return put3(out, 0xef, 0xbc, v[0]+0x60);
  }
  if (v[0] >= 0x60 && v[0] <= 0x7d) {
    return put3(out, 0xef, 0xbd, v[0]+0x20);
  }
  return 0;
}

static int lower_s(const uint8_t *v, size_t n, uint8_t *out) {
  (void)v;
  (void)n;
  return put1(out, 0x20);
}

static int upper_s(const uint8_t *v, size_t n, uint8_t *out) {
  (void)v;
  (void)n;
  return put3(out, 0xe3, 0x80, 0x80);
}

static int lower_k(const uint8_t *v, size_t n, uint8_t *out) {
  (void)n;
  uint8_t cval2;
  if (v[1] == 0x80) {
    switch (v[2]) {
    case 0x81: /* 、 -> ､ */
      return put3(out, 0xef, 0xbd, 0xa4);
    case 0x82: /* 。 -> ｡ */
      return put3(out, 0xef, 0xbd, 0xa1);
    }
  } else if (v[1] == 0x82) {
    switch (v[2]) {
    case 0x9b: /* ゛ ->  ﾞ */
      return put3(out, 0xef, 0xbe, 0x9e);
    case 0x9c: /* ゜ ->  ﾟ */
      return put3(out, 0xef, 0xbe, 0x9f);
    // ァ行: ァ -> ｧ, ィ -> ｨ, ゥ -> ｩ, ェ -> ｪ, ォ -> ｫ
    case 0xa1: case 0xa3: case 0xa5: case 0xa7: case 0xa9:
      cval2 = 0xa7 + (v[2]-0xa1)/0x02;
      return put3(out, 0xef, 0xbd, cval2);
    // ア行: ア -> ｱ, イ -> ｲ, ウ -> ｳ, エ -> ｴ, オ -> ｵ
    case 0xa2: case 0xa4: case 0xa6: case 0xa8: case 0xaa:
      cval2 = 0xb1 + (v[2]-0xa2)/0x02;
      return put3(out, 0xef, 0xbd, cval2);
    // カ行: カ -> ｶ, キ -> ｷ, ク -> ｸ, ケ -> ｹ, コ -> ｺ
    case 0xab: case 0xad: case 0xaf: case 0xb1: case 0xb3:
      cval2 = 0xb6 + (v[2]-0xab)/0x02;
      return put3(out, 0xef, 0xbd, cval2);
    // ガ行: ガ -> ｶﾞ, ギ -> ｷﾞ, グ -> ｸﾞ, ゲ -> ｹﾞ, ゴ -> ｺﾞ
    case 0xac: case 0xae: case 0xb0: case 0xb2: case 0xb4:
      cval2 = 0xb6 + (v[2]-0xab)/0x02;
      return put6(out, 0xef, 0xbd, cval2, 0xef, 0xbe, 0x9e);
    // サ行: サ -> ｻ, シ -> ｼ, ス -> ｽ, セ -> ｾ, ソ -> ｿ
    case 0xb5: case 0xb7: case 0xb9: case 0xbb: case 0xbd:
      cval2 = 0xbb + (v[2]-0xb5)/0x02;
      return put3(out, 0xef, 0xbd, cval2);
    // ザ行: ザ -> ｻﾞ, ジ -> ｼﾞ, ズ -> ｽﾞ, ゼ -> ｾﾞ, ゾ -> ｿﾞ
    case 0xb6: case 0xb8: case 0xba: case 0xbc: case 0xbe:
      cval2 = 0xbb + (v[2]-0xb5)/0x02;
      return put6(out, 0xef, 0xbd, cval2, 0xef, 0xbe, 0x9e);
      // タ行(タ): タ -> ﾀ
    case 0xbf:
      return put3(out, 0xef, 0xbe, 0x80);
    }
  } else if (v[1] == 0x83) {
    switch (v[2]) {
    case 0x81:
      // タ行(チ): チ -> ﾁ
      return put3(out, 0xef, 0xbe, 0x81);
    // ダ行(ダ,ヂ): ダ -> ﾀﾞ, ヂ -> ﾁﾞ
    case 0x80: case 0x82:
      cval2 = 0x80 + (v[2]-0x80)/0x02;
      return put6(out, 0xef, 0xbe, cval2, 0xef, 0xbe, 0x9e);
    // タ行(ッ): ッ -> ｯ
    case 0x83:
      return put3(out, 0xef, 0xbd, 0xaf);
    // タ行(ツ,テ,ト): ツ -> ﾂ, テ -> ﾃ, ト -> ﾄ
    case 0x84: case 0x86: case 0x88:
      cval2 = 0x82 + (v[2]-0x84)/0x02;
      return put3(out, 0xef, 0xbe, cval2);
    // ダ行(ヅ, デ, ド): ヅ -> ﾂﾞ, デ -> ﾃﾞ, ド -> ﾄﾞ
    case 0x85: case 0x87: case 0x89:
      cval2 = 0x82 + (v[2]-0x84)/0x02;
      return put6(out, 0xef, 0xbe, cval2, 0xef, 0xbe, 0x9e);
    // ナ行: ナ -> ﾅ, ニ -> ﾆ, ヌ -> ﾇ, ネ -> ﾈ, ノ -> ﾉ
    case 0x8a: case 0x8b: case 0x8c: case 0x8d: case 0x8e:
      cval2 = v[2] - 0x05;
      return put3(out, 0xef, 0xbe, cval2);
    // ハ行: ハ -> ﾊ, ヒ -> ﾋ, フ -> ﾌ, ヘ -> ﾍ, ホ -> ﾎ
    case 0x8f: case 0x92: case 0x95: case 0x98: case 0x9b:
      cval2 = 0x8a + (v[2]-0x8f)/0x03;
      return put3(out, 0xef, 0xbe, cval2);
    // バ行: バ -> ﾊﾞ, ビ -> ﾋﾞ, ブ -> ﾌﾞ, ベ -> ﾍﾞ, ボ -> ﾎﾞ
    case 0x90: case 0x93: case 0x96: case 0x99: case 0x9c:
      cval2 = 0x8a + (v[2]-0x8f)/0x03;
      return put6(out, 0xef, 0xbe, cval2, 0xef, 0xbe, 0x9e);
    // パ行: パ -> ﾊﾟ, ピ -> ﾋﾟ, プ -> ﾌﾟ, ペ -> ﾍﾟ, ポ -> ﾎﾟ
    case 0x91: case 0x94: case 0x97: case 0x9a: case 0x9d:
      cval2 = 0x8a + (v[2]-0x8f)/0x03;
      return put6(out, 0xef, 0xbe, cval2, 0xef, 0xbe, 0x9f);
    // マ行: マ -> ﾏ, ミ -> ﾐ, ム -> ﾑ, メ -> ﾒ, モ -> ﾓ
    case 0x9e: case 0x9f: case 0xa0: case 0xa1: case 0xa2:
      cval2 = v[2] - 0x0f;
      return put3(out, 0xef, 0xbe, cval2);
    // ャ行: ャ -> ｬ, ュ -> ｭ, ョ -> ｮ
    case 0xa3: case 0xa5: case 0xa7:
      cval2 = 0xac + (v[2]-0xa3)/0x02;
      return put3(out, 0xef, 0xbd, cval2);
    // ヤ行: ヤ -> ﾔ, ユ -> ﾕ, ヨ -> ﾖ
    case 0xa4: case 0xa6: case 0xa8:
      cval2 = 0x94 + (v[2]-0xa4)/0x02;
      return put3(out, 0xef, 0xbe, cval2);
    // ラ行: ラ -> ﾗ, リ -> ﾘ, ル -> ﾙ, レ -> ﾚ, ロ -> ﾛ
    case 0xa9: case 0xaa: case 0xab: case 0xac: case 0xad:
      cval2 = v[2] - 0x12;
      return put3(out, 0xef, 0xbe, cval2);
    // ヮ -> ﾜ, ワ -> ﾜ
    case 0xae: case 0xaf:
      return put3(out, 0xef, 0xbe, 0x9c);
    // ヰ -> ｲ
    case 0xb0:
      return put3(out, 0xef, 0xbd, 0xb2);
    // ヱ -> ｴ
    case 0xb1:
      return put3(out, 0xef, 0xbd, 0xb4);
    // ヲ -> ｦ
    case 0xb2:
      return put3(out, 0xef, 0xbd, 0xa6);
    // ン -> ﾝ
    case 0xb3:
      return put3(out, 0xef, 0xbe, 0x9d);
    // ヴ -> ｳﾞ
    case 0xb4:
      return put6(out, 0xef, 0xbd, 0xb3, 0xef, 0xbe, 0x9e);
    // ・ -> ･
    case 0xbb:
      return put3(out, 0xef, 0xbd, 0xa5);
    // ー -> ｰ
    case 0xbc:
      return put3(out, 0xef, 0xbd, 0xb0);
    }
  }
  return 0;
}

static int upper_k(const uint8_t *v, size_t n, uint8_t *out) {
  uint8_t cval2;
  if (v[1] == 0xbd) {
    switch (v[2]) {
    // ｡ -> 。
    case 0xa1:
      return put3(out, 0xe3, 0x80, 0x82);
    // ｢ -> 「, ｣ -> 」
    case 0xa2: case 0xa3:
      return put3(out, 0xe3, 0x80, v[2]-0x16);
    // ､ -> 、
    case 0xa4:
      return put3(out, 0xe3, 0x80, 0x81);
    // ･ -> ・
    case 0xa5:
      return put3(out, 0xe3, 0x83, 0xbb);
    // ｦ -> ヲ
    case 0xa6:
      return put3(out, 0xe3, 0x83, 0xb2);
    // ｧ行: ｧ -> ァ, ｨ -> ィ, ｩ -> ゥ, ｪ -> ェ, ｫ -> ォ
    case 0xa7: case 0xa8: case 0xa9: case 0xaa: case 0xab:
      cval2 = 0xa1 + (v[2]-0xa7)*0x02;
      return put3(out, 0xe3, 0x82, cval2);
    // ｬ行: ｬ -> ャ, ｭ -> ュ, ｮ -> ョ
    case 0xac: case 0xad: case 0xae:
      cval2 = 0xa3 + (v[2]-0xac)*0x02;
      return put3(out, 0xe3, 0x83, cval2);
    // ｯ -> ッ
    case 0xaf:
      return put3(out, 0xe3, 0x83, 0x83);
    // ｰ -> ー
    case 0xb0:
      return put3(out, 0xe3, 0x83, 0xbc);
    // ｱ行: ｱ -> ア, ｲ -> イ, ｴ -> エ, ｵ -> オ
    case 0xb1: case 0xb2: case 0xb4: case 0xb5:
      cval2 = 0xa2 + (v[2]-0xb1)*0x02;
      return put3(out, 0xe3, 0x82, cval2);
    // ｱ行(ｳ,ｳﾞ): ｳ -> ウ, ｳﾞ -> ヴ
    case 0xb3:
      if (n > 5 && v[5] == 0x9e) {
        return put3(out, 0xe3, 0x83, 0xb4);
      } else {
        return put3(out, 0xe3, 0x82, 0xa6);
      }
    // ｶ行: ｶ ->カ, ｷ -> キ, ｸ -> ク, ｹ -> ケ, ｺ -> コ
    // ｶﾞ行: ｶﾞ -> ガ, ｷﾞ -> ギ, ｸﾞ -> グ, ｹﾞ -> ゲ, ｺﾞ -> ゴ
    case 0xb6: case 0xb7: case 0xb8: case 0xb9: case 0xba:
      if (n > 5 && v[5] == 0x9e) {
        cval2 = 0xac + (v[2]-0xb6)*0x02;
      } else {
        cval2 = 0xab + (v[2]-0xb6)*0x02;
      }
      return put3(out, 0xe3, 0x82, cval2);
    // ｻ行: ｻ -> サ, ｼ -> シ, ｽ -> ス, ｾ -> セ, ｿ -> ソ
    // ｻﾞ行: ｻﾞ -> ザ, ｼﾞ -> ジ, ｽﾞ -> ズ, ｾﾞ -> ゼ, ｿﾞ -> ゾ
    case 0xbb: case 0xbc: case 0xbd: case 0xbe: case 0xbf:
      if (n > 5 && v[5] == 0x9e) {
        cval2 = 0xb6 + (v[2]-0xbb)*0x02;
      } else {
        cval2 = 0xb5 + (v[2]-0xbb)*0x02;
      }
      return put3(out, 0xe3, 0x82, cval2);
    }
  } else if (v[1] == 0xbe) {
    switch (v[2]) {
    // タ行(タ)・ダ行(ダ): ﾀ -> タ, ﾀﾞ -> ダ
    case 0x80:
      if (n > 5 && v[5] == 0x9e) {
        return put3(out, 0xe3, 0x83, 0x80);
      } else {
        return put3(out, 0xe3, 0x82, 0xbf);
      }
    // タ行(チ)・ダ行(ヂ): ﾁ -> チ, ﾁﾞ -> ヂ
    case 0x81:
      if (n > 5 && v[5] == 0x9e) {
        cval2 = 0x82;
      } else {
        cval2 = 0x81;
      }
      return put3(out, 0xe3, 0x83, cval2);
    // タ行(ツ,テ,ト)・ダ行(ヅ,デ,ド): ﾃ -> テ, ﾄ -> ト, ﾃﾞ -> デ, ﾄﾞ -> ド
    case 0x82: case 0x83: case 0x84:
      if (n > 5 && v[5] == 0x9e) {
        cval2 = 0x85 + (v[2]-0x82)*0x02;
      } else {
        cval2 = 0x84 + (v[2]-0x82)*0x02;
      }
      return put3(out, 0xe3, 0x83, cval2);
    // ナ行: (ﾅ -> ナ, ﾆ -> ニ, ﾇ -> ヌ, ﾈ -> ネ, ﾉ -> ノ)
    case 0x85: case 0x86: case 0x87: case 0x88: case 0x89:
      cval2 = v[2] + 0x05;
      return put3(out, 0xe3, 0x83, cval2);
    // ハ行: ﾊ -> ハ, ﾋ -> ヒ, ﾌ -> フ, ﾍ -> ヘ, ﾎ -> ホ
    // バ行: ﾊﾞ -> バ, ﾋﾞ -> ビ, ﾌﾞ -> ブ, ﾍﾞ -> ベ, ﾎﾞ -> ボ
    // パ行: ﾊﾟ -> パ, ﾋﾟ -> ピ, ﾌﾟ -> プ, ﾍﾟ -> ペ, ﾎﾟ -> ポ,
    case 0x8a: case 0x8b: case 0x8c: case 0x8d: case 0x8e:
      if (n > 5 && v[5] == 0x9e) {
        cval2 = 0x8a + (v[2]-0x88)*0x03;
      } else if (n > 5 && v[5] == 0x9f) {
        cval2 = 0x8b + (v[2]-0x88)*0x03;
      } else {
        cval2 = 0x89 + (v[2]-0x88)*0x03;
      }
      return put3(out, 0xe3, 0x83, cval2);
    // マ行: ﾏ -> マ, ﾐ -> ミ, ﾑ -> ム, ﾒ -> メ, ﾓ -> モ
    case 0x8f: case 0x90: case 0x91: case 0x92: case 0x93:
      cval2 = v[2] + 0x0f;
      return put3(out, 0xe3, 0x83, cval2);
    // ヤ行: ﾔ -> ヤ, ﾕ -> ユ, ﾖ -> ヨ
    case 0x94: case 0x95: case 0x96:
      cval2 = 0xa4 + (v[2]-0x94)*0x02;
      return put3(out, 0xe3, 0x83, cval2);
    case 0x97: case 0x98: case 0x99: case 0x9a: case 0x9b:
      cval2 = v[2] + 0x12;
      return put3(out, 0xe3, 0x83, cval2);
    // ﾜ -> ワ
    case 0x9c:
      return put3(out, 0xe3, 0x83, 0xaf);
    // ﾝ -> ン
    case 0x9d:
      return put3(out, 0xe3, 0x83, 0xb3);
    case 0x9e: /* ﾞ */
      return put3(out, 0xe3, 0x82, 0x9b);
    case 0x9f: /* ﾟ */
      return put3(out, 0xe3, 0x82, 0x9c);
    }
  }
  return 0;
}

static int lower_h(const uint8_t *v, size_t n, uint8_t *out) {
  (void)n;
  uint8_t cval2;
  if (v[1] == 0x80) {
    switch (v[2]) {
    // 、 -> ､
    case 0x81:
      return put3(out, 0xef, 0xbd, 0xa4);
    // 。 -> ｡
    case 0x82:
      return put3(out, 0xef, 0xbd, 0xa1);
    }
  } else if (v[1] == 0x81) {
    switch (v[2]) {
    // ぁ行: ぁ -> ｧ, ぃ -> ｨ, ぅ -> ｩ, ぇ -> ｪ, ぉ -> ｫ
    case 0x81: case 0x83: case 0x85: case 0x87: case 0x89:
      cval2 = 0xa7 + (v[2]-0x81)/0x02;
      return put3(out, 0xef, 0xbd, cval2);
    // あ行: あ -> ｱ, い -> ｲ, う -> ｳ, え -> ｴ, お -> ｵ
    case 0x82: case 0x84: case 0x86: case 0x88: case 0x8a:
      cval2 = 0xb1 + (v[2]-0x82)/0x02;
      return put3(out, 0xef, 0xbd, cval2);
    // か行: か -> ｶ, き -> ｷ, く -> ｸ, け -> ｹ, こ -> ｺ
    case 0x8b: case 0x8d: case 0x8f: case 0x91: case 0x93:
      cval2 = 0xb6 + (v[2]-0x8b)/0x02;
      return put3(out, 0xef, 0xbd, cval2);
    // が行: が -> ｶﾞ, ぎ -> ｷﾞ, ぐ -> ｸﾞ, げ -> ｹﾞ, ご -> ｺﾞ
    case 0x8c: case 0x8e: case 0x90: case 0x92: case 0x94:
      cval2 = 0xb6 + (v[2]-0x8c)/0x02;
      return put6(out, 0xef, 0xbd, cval2, 0xef, 0xbe, 0x9e);
    // さ行: さ -> ｻ, し -> ｼ, す -> ｽ, せ -> ｾ, そ -> ｿ
    case 0x95: case 0x97: case 0x99: case 0x9b: case 0x9d:
      cval2 = 0xbb + (v[2]-0x95)/0x02;
      return put3(out, 0xef, 0xbd, cval2);
    // ざ行: ざ -> ｻﾞ, じ -> ｼﾞ, ず -> ｽﾞ, ぜ -> ｾﾞ, ぞ -> ｿﾞ
    case 0x96: case 0x98: case 0x9a: case 0x9c: case 0x9e:
      cval2 = 0xbb + (v[2]-0x96)/0x02;
      return put6(out, 0xef, 0xbd, cval2, 0xef, 0xbe, 0x9e);
    // た行(た): た -> ﾀ
    case 0x9f:
      return put3(out, 0xef, 0xbe, 0x80);
    case 0xa1:
      // た行(ち): ち -> ﾁ
      return put3(out, 0xef, 0xbe, 0x81);
    // だ行(だ,ぢ): だ -> ﾀﾞ, ぢ -> ﾁﾞ
    case 0xa0: case 0xa2:
      cval2 = 0x80 + (v[2]-0xa0)/0x02;
      return put6(out, 0xef, 0xbe, cval2, 0xef, 0xbe, 0x9e);
    // た行(っ): っ -> ｯ
    case 0xa3:
      return put3(out, 0xef, 0xbd, 0xaf);
    // た行(つ,て,と): つ -> ﾂ, て -> ﾃ, と -> ﾄ
    case 0xa4: case 0xa6: case 0xa8:
      cval2 = 0x82 + (v[2]-0xa4)/0x02;
      return put3(out, 0xef, 0xbe, cval2);
    // だ行(づ,で,ど): づ -> ﾂﾞ, で -> ﾃﾞ, ど -> ﾄﾞ
    case 0xa5: case 0xa7: case 0xa9:
      cval2 = 0x82 + (v[2]-0xa4)/0x02;
      return put6(out, 0xef, 0xbe, cval2, 0xef, 0xbe, 0x9e);
    // な行: な -> ﾅ, に -> ﾆ, ぬ -> ﾇ, ね -> ﾈ, の -> ﾉ
    case 0xaa: case 0xab: case 0xac: case 0xad: case 0xae:
      cval2 = v[2] - 0x25;
      return put3(out, 0xef, 0xbe, cval2);

    // は行: は -> ﾊ, ひ -> ﾋ, ふ -> ﾌ, へ -> ﾍ, ほ -> ﾎ
    case 0xaf: case 0xb2: case 0xb5: case 0xb8: case 0xbb:
      cval2 = 0x8a + (v[2]-0xaf)/0x03;
      return put3(out, 0xef, 0xbe, cval2);
      // ば行: ば -> ﾊﾞ, び -> ﾋﾞ, ぶ -> ﾌﾞ, べ -> ﾍﾞ, ぼ -> ﾎﾞ
    case 0xb0: case 0xb3: case 0xb6: case 0xb9: case 0xbc:
      cval2 = 0x8a + (v[2]-0xaf)/0x03;
      return put6(out, 0xef, 0xbe, cval2, 0xef, 0xbe, 0x9e);
    // ぱ行: ぱ -> ﾊﾟ, ぴ -> ﾋﾟ, ぷ -> ﾌﾟ, ぺ -> ﾍﾟ, ぽ -> ﾎﾟ
    case 0xb1: case 0xb4: case 0xb7: case 0xba: case 0xbd:
      cval2 = 0x8a + (v[2]-0xaf)/0x03;
      return put6(out, 0xef, 0xbe, cval2, 0xef, 0xbe, 0x9f);
    // ま行(ま,み): ま -> ﾏ, み -> ﾐ
    case 0xbe: case 0xbf:
      cval2 = v[2] - 0x2f;
      return put3(out, 0xef, 0xbe, cval2);
    }
  } else if (v[1] == 0x82) {
    switch (v[2]) {
    // ま行(む,め,も): む -> ﾑ, め -> ﾒ, も -> ﾓ
    case 0x80: case 0x81: case 0x82:
      cval2 = v[2] + 0x11;
      return put3(out, 0xef, 0xbe, cval2);
    // ゃ行: ゃ -> ｬ, ゅ -> ｭ, ょ -> ｮ
    case 0x83: case 0x85: case 0x87:
      cval2 = 0xac + (v[2]-0x83)/0x02;
      return put3(out, 0xef, 0xbd, cval2);
    // や行: や -> ﾔ, ゆ -> ﾕ, よ -> ﾖ
    case 0x84: case 0x86: case 0x88:
      cval2 = 0x94 + (v[2]-0x84)/0x02;
      return put3(out, 0xef, 0xbe, cval2);
    // ら行: ら -> ﾗ, り -> ﾘ, る -> ﾙ, れ -> ﾚ, ろ -> ﾛ
    case 0x89: case 0x8a: case 0x8b: case 0x8c: case 0x8d:
      cval2 = v[2] + 0x0e;
      return put3(out, 0xef, 0xbe, cval2);
    // ゎ -> ﾜ, わ -> ﾜ
    case 0x8e: case 0x8f:
      return put3(out, 0xef, 0xbe, 0x9c);
    // ゐ -> ｲ
    case 0x90:
      return put3(out, 0xef, 0xbd, 0xb2);
    // ゑ -> ｴ
    case 0x91:
      return put3(out, 0xef, 0xbd, 0xb4);
    // を -> ｦ
    case 0x92:
      return put3(out, 0xef, 0xbd, 0xa6);
    // ン -> ﾝ
    case 0x93:
      return put3(out, 0xef, 0xbe, 0x9d);
    // ゛ -> ﾞ
    case 0x9b:
      return put3(out, 0xef, 0xbe, 0x9e);
    // ゜ -> ﾟ
    case 0x9c:
      return put3(out, 0xef, 0xbe, 0x9f);

    }
  } else if (v[1] == 0x83) {
    switch (v[2]) {
    // ・ -> ･
    case 0xbb:
      return put3(out, 0xef, 0xbd, 0xa5);
    // ー -> ｰ
    case 0xbc:
      return put3(out, 0xef, 0xbd, 0xb0);
    }
  }
  return 0;
}

static int upper_h(const uint8_t *v, size_t n, uint8_t *out) {
  uint8_t cval2;
  if (v[1] == 0xbd) {
    switch (v[2]) {
    // ｡ -> 。
    case 0xa1:
      return put3(out, 0xe3, 0x80, 0x82);
    // ｢ -> 「, ｣ -> 」
    case 0xa2: case 0xa3:
      cval2 = v[2] - 0x16;
      return put3(out, 0xe3, 0x80, cval2);
    // ､ -> 、
    case 0xa4:
      return put3(out, 0xe3, 0x80, 0x81);
    // ･ -> ・
    case 0xa5:
      return put3(out, 0xe3, 0x83, 0xbb);
    // ｦ -> を
    case 0xa6:
      return put3(out, 0xe3, 0x82, 0x92);
    // ｧ行: ｧ -> ぁ, ｨ -> ぃ, ｩ -> ぅ, ｪ -> ぇ, ｫ -> ぉ
    case 0xa7: case 0xa8: case 0xa9: case 0xaa: case 0xab:
      cval2 = 0x81 + (v[2]-0xa7)*0x02;
      return put3(out, 0xe3, 0x81, cval2);
    // ｬ行: ｬ -> ゃ, ｭ -> ゅ, ｮ -> ょ
    case 0xac: case 0xad: case 0xae:
      cval2 = 0x83 + (v[2]-0xac)*0x02;
      return put3(out, 0xe3, 0x82, cval2);
    // ﾀ行(ｯ): ｯ -> っ
    case 0xaf:
      return put3(out, 0xe3, 0x81, 0xa3);
    // ｰ -> ー
    case 0xb0: /* ｰ */
      return put3(out, 0xe3, 0x83, 0xbc);
    // ｱ行: ｱ -> あ, ｲ -> い, ｳ -> う, ｴ -> え, ｵ -> お
    case 0xb1: case 0xb2: case 0xb4: case 0xb5:
      cval2 = 0x82 + (v[2]-0xb1)*0x02;
      return put3(out, 0xe3, 0x81, cval2);
    case 0xb3:
      if (n > 5 && v[5] == 0x9e) {
        return put6(out, 0xe3, 0x81, 0x86, 0xe3, 0x82, 0x9b);
      }
      return put3(out, 0xe3, 0x81, 0x86);
    // ｶ行: ｶ -> か, ｷ -> き, ｸ -> く, ｹ -> け, ｺ -> こ
    // ｶﾞ行: ｶﾞ -> が, ｷﾞ -> ぎ, ｸﾞ -> ぐ, ｹﾞ -> げ, ｺﾞ -> ご
    case 0xb6: case 0xb7: case 0xb8: case 0xb9: case 0xba:
      if (n > 5 && v[5] == 0x9e) {
        cval2 = 0x8c + (v[2]-0xb6)*0x02;
      } else {
        cval2 = 0x8b + (v[2]-0xb6)*0x02;
      }
      return put3(out, 0xe3, 0x81, cval2);
    // ｻ行: ｻ -> さ, ｼ -> し, ｽ -> す, ｾ -> せ, ｿ -> そ
    // ｻﾞ行: ｻﾞ -> ざ, ｼﾞ -> じ, ｽﾞ -> ず, ｾﾞ -> ぜ, ｿﾞ -> ぞ
    case 0xbb: case 0xbc: case 0xbd: case 0xbe: case 0xbf:
      if (n > 5 && v[5] == 0x9e) {
        cval2 = 0x96 + (v[2]-0xbb)*0x02;
      } else {
        cval2 = 0x95 + (v[2]-0xbb)*0x02;
      }
      return put3(out, 0xe3, 0x81, cval2);
    }
  } else if (v[1] == 0xbe) {
    switch (v[2]) {
    // ﾀ行(ﾀ,ﾁ): ﾀ -> た, ﾁ -> ち
    // ﾀﾞ行(ﾀﾞ,ﾁﾞ): ﾀﾞ -> だ, ﾁﾞ -> ぢ
    case 0x80: case 0x81:
      if (n > 5 && v[5] == 0x9e) {
        cval2 = 0xa0 + (v[2]-0x80)*0x02;
      } else {
        cval2 = 0x9f + (v[2]-0x80)*0x02;
      }
      return put3(out, 0xe3, 0x81, cval2);
    // ﾀ行(ﾂ,ﾃ,ﾄ): ﾂ -> つ, ﾃ -> て, ﾄ -> と
    // ﾀﾞ行(ﾂﾞ,ﾃﾞ,ﾄﾞ): ﾂﾞ -> づ, ﾃﾞ -> で, ﾄﾞ -> ど
    case 0x82: case 0x83: case 0x84:
      if (n > 5 && v[5] == 0x9e) {
        cval2 = 0xa5 + (v[2]-0x82)*0x02;
      } else {
        cval2 = 0xa4 + (v[2]-0x82)*0x02;
      }
      return put3(out, 0xe3, 0x81, cval2);
    // ﾅ行: ﾅ -> な, ﾆ -> に, ﾇ -> ぬ, ﾈ -> ね, ﾉ -> の
    case 0x85: case 0x86: case 0x87: case 0x88: case 0x89:
      cval2 = v[2] + 0x25;
      return put3(out, 0xe3, 0x81, cval2);
      // ﾊ行: ﾊ -> は, ﾋ -> ひ, ﾌ -> ふ, ﾍ -> へ, ﾎ -> ほ
      // ﾊﾞ行: ﾊﾞ -> ば, ﾋﾞ -> び, ﾌﾞ -> ぶ, ﾍﾞ -> べ, ﾎﾞ -> ぼ
      // ﾊﾟ行: ﾊﾟ -> ぱ, ﾋﾟ -> ぴ, ﾌﾟ -> ぷ, ﾍﾟ -> ぺ, ﾎﾟ -> ぽ
    case 0x8a: case 0x8b: case 0x8c: case 0x8d: case 0x8e:
      if (n > 5 && v[5] == 0x9e) {
        cval2 = 0xb0 + (v[2]-0x8a)*0x03;
      } else if (n > 5 && v[5] == 0x9f) {
        cval2 = 0xb1 + (v[2]-0x8a)*0x03;
      } else {
        cval2 = 0xaf + (v[2]-0x8a)*0x03;
      }
      return put3(out, 0xe3, 0x81, cval2);
    // ﾏ行: ﾏ -> ま, ﾐ -> み
    case 0x8f: case 0x90:
      cval2 = v[2] + 0x2f;
      return put3(out, 0xe3, 0x81, cval2);
    // ﾏ行: ﾑ -> む, ﾒ -> め, ﾓ -> も
    case 0x91: case 0x92: case 0x93:
      cval2 = v[2] - 0x11;
      return put3(out, 0xe3, 0x82, cval2);
    // ﾔ行: ﾔ -> や, ﾕ -> ゆ, ﾖ -> よ
    case 0x94: case 0x95: case 0x96:
      cval2 = 0x84 + (v[2]-0x94)*0x02;
      return put3(out, 0xe3, 0x82, cval2);
    // ﾗ行: ﾗ -> ら, ﾘ -> り, ﾙ -> る, ﾚ -> れ, ﾛ -> ろ
    case 0x97: case 0x98: case 0x99: case 0x9a: case 0x9b:
      cval2 = v[2] - 0x0e;
      return put3(out, 0xe3, 0x82, cval2);
    // ﾜ行: ﾜ -> わ
    case 0x9c:
      return put3(out, 0xe3, 0x82, 0x8f);
    // ﾝ -> ん
    case 0x9d:
      return put3(out, 0xe3, 0x82, 0x93);
    // ﾞ -> ゛
    case 0x9e:
      return put3(out, 0xe3, 0x82, 0x9b);
    // ﾟ -> ゜
    case 0x9f:
      return put3(out, 0xe3, 0x82, 0x9c);
    }
  }
  return 0;
}

static int lower_c(const uint8_t *v, size_t n, uint8_t *out) {
  (void)n;
  switch (v[1]) {
  case 0x82: // ァ - タ
    if (v[2] >= 0xa1 && v[2] <= 0xbf) {
      return put3(out, 0xe3, 0x81, v[2]-0x20);
    }
    break;
  case 0x83:
    if (v[2] >= 0x80 && v[2] <= 0x9f) { // ダ - ミ
      return put3(out, 0xe3, 0x81, v[2]+0x20);
    } else if (v[2] >= 0xa0 && v[2] <= 0xb3) { // ム - ン
      return put3(out, 0xe3, 0x82, v[2]-0x20);
    } else if (v[2] >= 0xbd && v[2] <= 0xbe) { // ヽヾ
      return put3(out, 0xe3, 0x82, v[2]-0x20);
    }
  }
  return 0;
}

static int upper_c(const uint8_t *v, size_t n, uint8_t *out) {
  (void)n;
  switch (v[1]) {
  case 0x81:
    if (v[2] >= 0x81 && v[2] <= 0x9f) { // ぁ - た
      return put3(out, 0xe3, 0x82, v[2]+0x20);
    } else if (v[2] >= 0xa0 && v[2] <= 0xbf) { // だ - み
      return put3(out, 0xe3, 0x83, v[2]-0x20);
    }
    break;
  case 0x82:
    if (v[2] >= 0x80 && v[2] <= 0x93) { // む - ん
      return put3(out, 0xe3, 0x83, v[2]+0x20);
    } else if (v[2] >= 0x9d && v[2] <= 0x9e) { // ゝゞ
      return put3(out, 0xe3, 0x83, v[2]+0x20);
    }
  }
  return 0;
}

static int mode_filter(char m) {
  switch (m) {
    case 'r': return CNV_LOWER_R;
    case 'R': return CNV_UPPER_R;
    case 'n': return CNV_LOWER_N;
    case 'N': return CNV_UPPER_N;
    case 'a': return CNV_LOWER_A;
    case 'A': return CNV_UPPER_A;
    case 's': return CNV_LOWER_S;
    case 'S': return CNV_UPPER_S;
    case 'k': return CNV_LOWER_K;
    case 'K': return CNV_UPPER_K;
    case 'h': return CNV_LOWER_H;
    case 'H': return CNV_UPPER_H;
    case 'c': return CNV_LOWER_C;
    case 'C': return CNV_UPPER_C;
  }
  return CNV_ASIS;
}

static void parse_mode(converter *cv, const char *mode, size_t mode_len) {
  cv->n = 0;
  cv->mask = 0;
  for (size_t i = 0; i < mode_len; i++) {
    int f = mode_filter(mode[i]);
    if (f == CNV_ASIS || (cv->mask & f)) {
      continue;
    }
    cv->filters[cv->n++] = f;
    cv->mask |= f;
  }
}

/*
 * Applies the applicable filters to the character v (n bytes); the last one
 * in mode order that produces output wins. Returns the output length, or 0
 * when the character is left as is.
 */
static int convert_char(const converter *cv, const uint8_t *v, size_t n,
                        int flags, uint8_t *out) {
  for (int j = cv->n - 1; j >= 0; j--) {
    int f = cv->filters[j];
    if (!(flags & f)) {
      continue;
    }
    int k = 0;
    switch (f) {
      case CNV_LOWER_R: k = lower_r(v, n, out); break;
      case CNV_UPPER_R: k = upper_r(v, n, out); break;
      case CNV_LOWER_N: k = lower_n(v, n, out); break;
      case CNV_UPPER_N: k = upper_n(v, n, out); break;
      case CNV_LOWER_A: k = lower_a(v, n, out); break;
      case CNV_UPPER_A: k = upper_a(v, n, out); break;
      case CNV_LOWER_S: k = lower_s(v, n, out); break;
      case CNV_UPPER_S: k = upper_s(v, n, out); break;
      case CNV_LOWER_K: k = lower_k(v, n, out); break;
      case CNV_UPPER_K: k = upper_k(v, n, out); break;
      case CNV_LOWER_H: k = lower_h(v, n, out); break;
      case CNV_UPPER_H: k = upper_h(v, n, out); break;
      case CNV_LOWER_C: k = lower_c(v, n, out); break;
      case CNV_UPPER_C: k = upper_c(v, n, out); break;
    }
    if (k > 0) {
      return k;
    }
  }
  return 0;
}

char *kanaco_convert(const char *str, size_t str_len, const char *mode,
                     size_t mode_len, size_t *out_len) {
  converter cv;
  parse_mode(&cv, mode, mode_len);
  if (mode_len == 0) {
    str_len = 0;
  }

  /*
   * A character grows at most 3 times (1 byte -> 3 bytes, or 3 bytes ->
   * 6 bytes), so one allocation is enough.
   */
  if (str_len > (SIZE_MAX - 1) / 3) {
    return NULL;
  }
  size_t cap = cv.mask ? str_len * 3 : str_len;
  uint8_t *buf = (uint8_t *)malloc(cap + 1);
  if (buf == NULL) {
    return NULL;
  }

  const uint8_t *s = (const uint8_t *)str;
  size_t w = 0;
  if (cv.mask == 0) {
    if (str_len > 0) {
      memcpy(buf, s, str_len);
    }
    w = str_len;
  } else {
    if (!ascii_table_ready) {
      init_ascii_table();
    }
    /* Characters no filter touches are copied in contiguous runs. */
    size_t start = 0;
    for (size_t i = 0; i < str_len;) {
      size_t size = 1;
      int flags;
      if (s[i] < 0x80) {
        flags = ascii_table[s[i]];
      } else {
        size = extract(s + i, str_len - i, &flags);
      }
      if (flags & cv.mask) {
        uint8_t out[BUF_CHARS];
        int k = convert_char(&cv, s + i, size, flags & cv.mask, out);
        if (k > 0) {
          memcpy(buf + w, s + start, i - start);
          w += i - start;
          memcpy(buf + w, out, (size_t)k);
          w += (size_t)k;
          start = i + size;
        }
      }
      i += size;
    }
    memcpy(buf + w, s + start, str_len - start);
    w += str_len - start;

    /* Give back the unused part of the worst-case allocation. */
    uint8_t *tmp = (uint8_t *)realloc(buf, w + 1);
    if (tmp != NULL) {
      buf = tmp;
    }
  }
  buf[w] = 0x00;
  if (out_len != NULL) {
    *out_len = w;
  }
  return (char *)buf;
}

char *convert(const char *str, int str_len, const char *mode, int mode_len) {
  if (str_len < 0 || mode_len < 0) {
    return NULL;
  }
  return kanaco_convert(str, (size_t)str_len, mode, (size_t)mode_len, NULL);
}
