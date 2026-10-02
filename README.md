# KanaCo

KanaCo is a library for converting Japanese text between full-width (zenkaku) and half-width (hankaku) forms, and between hiragana and katakana. It is inspired by PHP's [`mb_convert_kana`](https://www.php.net/manual/en/function.mb-convert-kana.php) and uses the same mode letters.

It is available for Go, C and Python (through Cython). All three produce identical output.

```go
kanaco.String("ｶﾅｺ　ｺﾝﾊﾞｰﾀｰ　Ｖｅｒ１", "Kas") // "カナコ コンバーター Ver1"
```

- Works on UTF-8 input. In Go it accepts `string`, `[]byte`, or a stream through `io.Reader`.
- Joins a half-width kana and its voiced mark into one character (`ｶﾞ` → `ガ`, `ﾊﾟ` → `パ`).
- Leaves characters that the selected modes don't cover unchanged, including kanji, other scripts and invalid UTF-8 bytes.
- Has no dependencies outside the Go or C standard library.

## Contents

- [Go](#go)
  - [Installation](#installation)
  - [Quick start](#quick-start)
  - [API](#api)
- [C](#c)
- [Python](#python)
- [Modes](#modes)
- [Combining modes](#combining-modes)
- [Details and limitations](#details-and-limitations)
- [Development](#development)
- [License](#license)

## Go

### Installation

```sh
go get github.com/tsukinoha/kanaco
```

Go 1.25 or later is required.

### Quick start

```go
package main

import (
	"fmt"

	"github.com/tsukinoha/kanaco"
)

func main() {
	// Full-width letters and digits to half-width
	fmt.Println(kanaco.String("１２３ａｂｃＡＢＣ", "a")) // 123abcABC

	// Half-width katakana to full-width katakana, voiced marks included
	fmt.Println(kanaco.String("ｺﾝﾊﾞｰﾀｰ", "K")) // コンバーター

	// Hiragana to katakana
	fmt.Println(kanaco.String("かなこ", "C")) // カナコ

	// Several conversions at once: full-width letters/digits to half-width,
	// full-width spaces to half-width, half-width katakana to full-width
	fmt.Println(kanaco.String("ｶﾅｺ　ｺﾝﾊﾞｰﾀｰ　Ｖｅｒ１", "Kas")) // カナコ コンバーター Ver1
}
```

### API

#### `func String(str, mode string) string`

Converts `str` according to `mode` and returns the result.

```go
s := kanaco.String("かなこ", "C") // "カナコ"
```

#### `func Byte(b []byte, mode string) []byte`

Converts `b` according to `mode` and returns the result in a new slice. `b` itself is not modified.

```go
out := kanaco.Byte([]byte("123abcABC １２３ａｂｃＡＢＣ"), "RNS")
fmt.Printf("%s\n", out) // １２３ａｂｃＡＢＣ　１２３ａｂｃＡＢＣ
```

#### `func NewReader(r io.Reader, mode string) *Reader`

Returns a `*Reader` that converts the text read from `r`. Use it for files or other large inputs that you don't want to load into memory at once.

```go
f, err := os.Open("input.txt")
if err != nil {
	log.Fatal(err)
}
defer f.Close()

r := kanaco.NewReader(f, "Kas")
if _, err := io.Copy(os.Stdout, r); err != nil {
	log.Fatal(err)
}
```

`Reader` processes its input one line at a time:

- Each `Read` call returns exactly one converted line, including its trailing `\n`. A final line without a newline is also returned.
- The buffer `p` passed to `Read` must be large enough to hold the whole converted line. Otherwise `Read` returns the error `buffer size is not enough`, and that line is lost. A converted line can be up to 3 times as long as the input line, because half-width characters become 3-byte full-width characters. `io.Copy` uses a 32 KiB buffer, which is enough unless lines are very long.
- After the last line, `Read` returns `0, io.EOF`.

If you call `Read` yourself, use only the first `n` bytes of the buffer:

```go
buf := make([]byte, 64*1024)
for {
	n, err := r.Read(buf)
	if err == io.EOF {
		break
	}
	if err != nil {
		log.Fatal(err)
	}
	os.Stdout.Write(buf[:n])
}
```

## C

The C library is in `c/`. It needs a C99 compiler such as gcc and has no other dependencies.

### Building

```sh
make -C c
```

This builds the static library `c/libkanaco.a` and the shared library `c/libkanaco.so`. The header is `c/kanaco.h`.

### Usage

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kanaco.h"

int main(void) {
  const char *in = "ｶﾅｺ　ｺﾝﾊﾞｰﾀｰ　Ｖｅｒ１";
  size_t out_len;
  char *out = kanaco_convert(in, strlen(in), "Kas", 3, &out_len);
  if (out == NULL) {
    perror("kanaco_convert");
    return 1;
  }
  printf("%s (%zu bytes)\n", out, out_len);  // カナコ コンバーター Ver1 (33 bytes)
  free(out);
  return 0;
}
```

To link with the static library:

```sh
gcc -I c -o example example.c c/libkanaco.a
```

To link with the shared library, `libkanaco.so` must also be found at run time, for example through `LD_LIBRARY_PATH`:

```sh
gcc -I c -o example example.c -L c -lkanaco
LD_LIBRARY_PATH=c ./example
```

### Functions

#### `char *kanaco_convert(const char *str, size_t str_len, const char *mode, size_t mode_len, size_t *out_len)`

Converts the `str_len` bytes at `str` according to the `mode_len` bytes at `mode`.

- Returns a newly allocated, NUL-terminated string. The caller must release it with `free()`.
- Stores the length of the result in bytes in `*out_len`. Pass `NULL` if you don't need it.
- Returns `NULL` only when memory cannot be allocated.

`str` does not have to be NUL-terminated and may contain NUL bytes. If it does, use `out_len` instead of `strlen()` to get the length of the result.

#### `char *convert(const char *str, int str_len, const char *mode, int mode_len)`

The older interface, kept for compatibility. It works like `kanaco_convert()` but does not return the length of the result. It returns `NULL` if `str_len` or `mode_len` is negative.

## Python

A Python module built with Cython is in `cython/`.

### Building

You need gcc, Cython 3, and the Python development headers. `python3-config` must be available; on Debian and Ubuntu it is in the `python3-dev` package.

```sh
make cython
```

This builds the extension module `cython/kanaco.cpython-<version>-<platform>.so` (for example `kanaco.cpython-313-x86_64-linux-gnu.so`). There is no pip package yet. To use the module, add `cython/` to `PYTHONPATH` or copy the `.so` file next to your code:

```sh
PYTHONPATH=cython python3 -c 'import kanaco; print(kanaco.conv("ｶﾅｺ", "K"))'
```

### Usage

```python
import kanaco

kanaco.conv("ｶﾅｺ　ｺﾝﾊﾞｰﾀｰ　Ｖｅｒ１", "Kas")  # 'カナコ コンバーター Ver1'
kanaco.conv("かなこ", "C")                 # 'カナコ'
kanaco.conv("ｶﾅｺ".encode(), "K")           # 'カナコ'  (bytes input)
kanaco.conv(2026, "N")                     # '２０２６'  (int input)
```

#### `conv(s, m: str) -> str`

Converts `s` according to the mode `m` and returns a `str`.

- `s` may be a `str`, `bytes` (UTF-8) or `int`. An `int` is converted with `str()` first. Any other type, including subclasses of these types, raises `TypeError`.
- `m` must be a `str`.
- The result is always a `str`, even for `bytes` input. Invalid UTF-8 bytes in a `bytes` input are dropped from the result.
- The GIL is released during the conversion, so other threads can run while a large text is converted.

## Modes

The mode letters are the same in Go, C and Python.

A mode is a string made of one or more of the letters below. As a rule, a lowercase letter converts to half-width and an uppercase letter converts to full-width. `c` and `C` are the exceptions: they switch between hiragana and katakana without changing the width.

| Mode | Converts | Example |
|---|---|---|
| `r` | Full-width letters → half-width | `ａｂｃＡＢＣ` → `abcABC` |
| `R` | Half-width letters → full-width | `abcABC` → `ａｂｃＡＢＣ` |
| `n` | Full-width digits → half-width | `１２３` → `123` |
| `N` | Half-width digits → full-width | `123` → `１２３` |
| `a` | Full-width letters, digits and symbols → half-width | `１ａＡ！＃` → `1aA!#` |
| `A` | Half-width letters, digits and symbols → full-width | `1aA!#` → `１ａＡ！＃` |
| `s` | Full-width space (U+3000) → half-width space (U+0020) | `"a　b"` → `"a b"` |
| `S` | Half-width space (U+0020) → full-width space (U+3000) | `"a b"` → `"a　b"` |
| `k` | Full-width katakana → half-width katakana | `コンバーター` → `ｺﾝﾊﾞｰﾀｰ` |
| `K` | Half-width katakana → full-width katakana | `ｺﾝﾊﾞｰﾀｰ` → `コンバーター` |
| `h` | Full-width hiragana → half-width katakana | `こんばーたー` → `ｺﾝﾊﾞｰﾀｰ` |
| `H` | Half-width katakana → full-width hiragana | `ｺﾝﾊﾞｰﾀｰ` → `こんばーたー` |
| `c` | Full-width katakana → full-width hiragana | `カナコ` → `かなこ` |
| `C` | Full-width hiragana → full-width katakana | `かなこ` → `カナコ` |

Notes on specific modes:

- **`a` / `A`** cover the range U+0021–U+007D (`!` to `}`) and its full-width counterparts, except `"` (U+0022), `'` (U+0027), `\` (U+005C) and `~` (U+007E). The space is not included. Use `s` / `S` for spaces.
- **`k` / `h`** also convert the Japanese punctuation `、。・ー゛゜` to half-width (`､｡･ｰﾞﾟ`). Voiced kana become two half-width characters (`ガ` → `ｶﾞ`, `パ` → `ﾊﾟ`).
- **`K` / `H`** also convert the half-width punctuation `｡｢｣､･ｰ` to full-width (`。「」、・ー`).
- **`k`** maps characters that have no half-width form to the closest one: `ヰ` → `ｲ`, `ヱ` → `ｴ`, `ヮ` → `ﾜ`. These conversions cannot be reversed. `ヵ` and `ヶ` are left unchanged.
- **`H`** turns `ｳﾞ` into the two characters `う゛`, because the precomposed `ゔ` is not used. `K` turns `ｳﾞ` into `ヴ`.
- **`c` / `C`** also convert the iteration marks `ヽヾ` ↔ `ゝゞ`. `ヴ`, `ヵ` and `ヶ` are not converted by `c`.

## Combining modes

You can put several letters in one mode string. Each character is converted at most once.

```go
kanaco.String("ｶﾅｺ　Ｖｅｒ１", "Kas") // "カナコ Ver1"
```

If two modes apply to the same character, the one that appears **later** in the mode string is used:

```go
kanaco.String("ｶﾅ", "KH")  // "かな"  (H comes later, so hiragana)
kanaco.String("ｶﾅ", "HK")  // "カナ"  (K comes later, so katakana)
kanaco.String("カナ", "kc") // "かな"  (c comes later)
kanaco.String("カナ", "ck") // "ｶﾅ"   (k comes later)
```

The conversions are not chained. With `"Ck"`, for example, hiragana becomes full-width katakana and is not then converted again to half-width.

Repeated letters are ignored (`"KK"` is the same as `"K"`), and so are letters that are not in the table above.

## Details and limitations

- **Encoding:** The input must be UTF-8. Other encodings such as Shift_JIS or EUC-JP are not detected, and the output for them is not guaranteed. Convert such text to UTF-8 first (in Go, for example with `golang.org/x/text/encoding/japanese`).
- **Empty mode:** An empty mode returns an empty result, not the input. This applies to `String(s, "")` and `Byte(b, "")` in Go, `kanaco_convert()` in C, and `conv(s, "")` in Python. A mode that contains only unknown letters (such as `"xyz"`) returns the input unchanged.
- **Unsupported characters:** Kanji, other scripts, emoji and invalid byte sequences are copied to the output as they are.

## Development

Run the Go tests:

```sh
go test ./...
```

Build the C library and the Python module, and run their tests (the Python tests need pytest):

```sh
make
make test
```

`make clean` removes the build outputs.

The test data is in `data/`. `data/input.txt` is the input. Each `data/output.<modes>.txt` file holds the expected output for one mode combination. In the file names, `l` stands for a lowercase mode letter and `u` for an uppercase one. For example, `output.ls.ua.uk.txt` is the expected output for the mode `"sAK"`.

All three implementations are tested against the same files.

## License

KanaCo is distributed under the MIT License. See [LICENSE](LICENSE) or https://opensource.org/licenses/mit-license.php.
