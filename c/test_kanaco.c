#define _POSIX_C_SOURCE 200809L

#include <glob.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kanaco.h"

static int failures = 0;

#define CHECK(cond, ...)               \
  do {                                 \
    if (!(cond)) {                     \
      fprintf(stderr, "FAIL: ");       \
      fprintf(stderr, __VA_ARGS__);    \
      fprintf(stderr, "\n");           \
      failures++;                      \
    }                                  \
  } while (0)

static char *read_file(const char *path, size_t *len) {
  FILE *f = fopen(path, "rb");
  if (f == NULL) {
    return NULL;
  }
  size_t cap = 4096, n = 0, r;
  char *buf = malloc(cap);
  while (buf != NULL && (r = fread(buf + n, 1, cap - n, f)) > 0) {
    n += r;
    if (n == cap) {
      char *tmp = realloc(buf, cap *= 2);
      if (tmp == NULL) {
        free(buf);
      }
      buf = tmp;
    }
  }
  fclose(f);
  *len = n;
  return buf;
}

/* output.ls.ua.uk.txt -> "sAK" ("l" = lowercase, "u" = uppercase) */
static void mode_from_path(const char *path, char *mode, size_t size) {
  char *tmp = strdup(path);
  char *name = basename(tmp) + strlen("output.");
  size_t n = 0;
  for (char *tok = strtok(name, "."); tok != NULL && n + 1 < size;
       tok = strtok(NULL, ".")) {
    if (strlen(tok) == 2 && (tok[0] == 'l' || tok[0] == 'u')) {
      mode[n++] = tok[0] == 'u' ? (char)(tok[1] - 'a' + 'A') : tok[1];
    }
  }
  mode[n] = '\0';
  free(tmp);
}

static void test_data_files(const char *dir) {
  char path[4096];
  snprintf(path, sizeof(path), "%s/input.txt", dir);
  size_t in_len;
  char *in = read_file(path, &in_len);
  CHECK(in != NULL, "cannot read %s", path);
  if (in == NULL) {
    return;
  }

  glob_t g;
  snprintf(path, sizeof(path), "%s/output.*.txt", dir);
  CHECK(glob(path, 0, NULL, &g) == 0 && g.gl_pathc > 0, "no files match %s",
        path);
  for (size_t i = 0; i < g.gl_pathc; i++) {
    char mode[32];
    mode_from_path(g.gl_pathv[i], mode, sizeof(mode));
    size_t exp_len, out_len;
    char *exp = read_file(g.gl_pathv[i], &exp_len);
    char *out = kanaco_convert(in, in_len, mode, strlen(mode), &out_len);
    CHECK(exp != NULL && out != NULL && exp_len == out_len &&
              memcmp(exp, out, out_len) == 0,
          "[%s] %s", mode, g.gl_pathv[i]);
    free(exp);
    free(out);
  }
  printf("checked %zu data files\n", (size_t)g.gl_pathc);
  globfree(&g);
  free(in);
}

static void check_str(const char *in, const char *mode, const char *expect) {
  size_t out_len;
  char *out = kanaco_convert(in, strlen(in), mode, strlen(mode), &out_len);
  CHECK(out != NULL && out_len == strlen(expect) && strcmp(out, expect) == 0,
        "[%s] %s -> %s (expect %s)", mode, in, out ? out : "(null)", expect);
  free(out);
}

static void test_cases(void) {
  check_str("ｶﾅｺ　ｺﾝﾊﾞｰﾀｰ　Ｖｅｒ１", "Kas", "カナコ コンバーター Ver1");
  check_str("ｶﾞｷﾞﾊﾟｳﾞ", "K", "ガギパヴ");
  check_str("ｳﾞ", "H", "う゛");
  check_str("ｶﾅ", "KH", "かな");
  check_str("ｶﾅ", "HK", "カナ");
  check_str("abc", "xyz", "abc");
  check_str("abc", "", "");
  check_str("", "K", "");
  /* truncated and invalid sequences pass through */
  check_str("ｶﾞ\xff\xe3", "K", "ガ\xff\xe3");
}

static void test_nul_bytes(void) {
  const char in[] = {'a', '\0', 'b'};
  size_t out_len;
  char *out = kanaco_convert(in, sizeof(in), "A", 1, &out_len);
  CHECK(out != NULL && out_len == 7 && memcmp(out, "ａ\0ｂ", 7) == 0,
        "NUL byte in the middle of the input");
  free(out);
}

static void test_compat(void) {
  const char *in = "ｶﾅｺ";
  char *out = convert(in, (int)strlen(in), "K", 1);
  CHECK(out != NULL && strcmp(out, "カナコ") == 0, "convert()");
  free(out);
  CHECK(convert(in, -1, "K", 1) == NULL, "convert() with negative length");
}

int main(int argc, char **argv) {
  test_data_files(argc > 1 ? argv[1] : "../data");
  test_cases();
  test_nul_bytes();
  test_compat();
  if (failures > 0) {
    printf("%d failure(s)\n", failures);
    return 1;
  }
  printf("ok\n");
  return 0;
}
