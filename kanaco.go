package kanaco

import (
	"bufio"
	"fmt"
	"io"
	"unsafe"
)

const (
	BufChars    int = 6
	FLT_ASIS    int = 0
	FLT_LOWER_R int = 1 << 0
	FLT_UPPER_R int = 1 << 1
	FLT_LOWER_N int = 1 << 2
	FLT_UPPER_N int = 1 << 3
	FLT_LOWER_A int = 1 << 4
	FLT_UPPER_A int = 1 << 5
	FLT_LOWER_S int = 1 << 6
	FLT_UPPER_S int = 1 << 7
	FLT_LOWER_K int = 1 << 8
	FLT_UPPER_K int = 1 << 9
	FLT_LOWER_H int = 1 << 10
	FLT_UPPER_H int = 1 << 11
	FLT_LOWER_C int = 1 << 12
	FLT_UPPER_C int = 1 << 13
)

type (
	Reader struct {
		r    *bufio.Reader
		mode string
		cv   converter
		line []byte // reused when a line exceeds the bufio buffer
		out  []byte // reused conversion buffer
	}
	// converter holds a parsed mode: the filters in the order they were
	// given (a later filter wins when several apply) and their union mask.
	converter struct {
		filters [14]int
		n       int
		mask    int
	}
)

func Byte(b []byte, mode string) []byte {
	if len(mode) == 0 {
		return []byte{}
	}
	cv := newConverter(mode)
	return cv.appendTo(make([]byte, 0, len(b)), b)
}

func String(str, mode string) string {
	if len(mode) == 0 || len(str) == 0 {
		return ""
	}
	// The input is only read, and the result buffer is never shared, so both
	// conversions can skip the copy.
	b := Byte(unsafe.Slice(unsafe.StringData(str), len(str)), mode)
	if len(b) == 0 {
		return ""
	}
	return unsafe.String(unsafe.SliceData(b), len(b))
}

func NewReader(r io.Reader, mode string) *Reader {
	reader := new(Reader)
	reader.r = bufio.NewReader(r)
	reader.mode = mode
	reader.cv = newConverter(mode)
	return reader
}

func (r *Reader) Read(p []byte) (int, error) {
	line, err := r.r.ReadSlice('\n')
	if err == bufio.ErrBufferFull {
		r.line = append(r.line[:0], line...)
		for err == bufio.ErrBufferFull {
			line, err = r.r.ReadSlice('\n')
			r.line = append(r.line, line...)
		}
		line = r.line
	}
	// A last line without a trailing newline comes with io.EOF; convert it
	// now and report io.EOF on the next call.
	// A last line without a trailing newline comes with io.EOF; convert it
	// now and report io.EOF on the next call.
	if err != nil && (err != io.EOF || len(line) == 0) {
		return 0, err
	}
	if len(r.mode) == 0 {
		return 0, nil
	}
	r.out = r.cv.appendTo(r.out[:0], line)
	if len(p) < len(r.out) {
		return 0, fmt.Errorf("buffer size is not enough")
	}
	n := copy(p, r.out)
	return n, nil
}

// -------------------------------------

func newConverter(mode string) converter {
	var cv converter
	for i := 0; i < len(mode); i++ {
		f := modeFilter(mode[i])
		if f == FLT_ASIS || cv.mask&f != 0 {
			continue
		}
		cv.filters[cv.n] = f
		cv.n++
		cv.mask |= f
	}
	return cv
}

func modeFilter(m byte) int {
	switch m {
	case 'r':
		return FLT_LOWER_R
	case 'R':
		return FLT_UPPER_R
	case 'n':
		return FLT_LOWER_N
	case 'N':
		return FLT_UPPER_N
	case 'a':
		return FLT_LOWER_A
	case 'A':
		return FLT_UPPER_A
	case 's':
		return FLT_LOWER_S
	case 'S':
		return FLT_UPPER_S
	case 'k':
		return FLT_LOWER_K
	case 'K':
		return FLT_UPPER_K
	case 'h':
		return FLT_LOWER_H
	case 'H':
		return FLT_UPPER_H
	case 'c':
		return FLT_LOWER_C
	case 'C':
		return FLT_UPPER_C
	}
	return FLT_ASIS
}

// appendTo appends the conversion of b to dst. Characters that no filter
// touches are copied in contiguous runs instead of one by one.
func (cv *converter) appendTo(dst, b []byte) []byte {
	mask := cv.mask
	if mask == 0 {
		return append(dst, b...)
	}
	var out [BufChars]byte
	start := 0
	for i := 0; i < len(b); {
		size, flags := 1, FLT_ASIS
		if c0 := b[i]; c0 < 0x80 {
			flags = asciiFlags[c0]
		} else {
			size, flags = extract(b[i:])
		}
		if flags&mask != 0 {
			if n := cv.convert(b[i:i+size], flags&mask, &out); n > 0 {
				dst = append(dst, b[start:i]...)
				dst = append(dst, out[:n]...)
				start = i + size
			}
		}
		i += size
	}
	return append(dst, b[start:]...)
}

// convert applies the applicable filters; the last one in mode order that
// produces output wins.
func (cv *converter) convert(v []byte, flags int, out *[BufChars]byte) int {
	for j := cv.n - 1; j >= 0; j-- {
		f := cv.filters[j]
		if flags&f == 0 {
			continue
		}
		var n int
		switch f {
		case FLT_LOWER_R:
			n = lowerR(v, out)
		case FLT_UPPER_R:
			n = upperR(v, out)
		case FLT_LOWER_N:
			n = lowerN(v, out)
		case FLT_UPPER_N:
			n = upperN(v, out)
		case FLT_LOWER_A:
			n = lowerA(v, out)
		case FLT_UPPER_A:
			n = upperA(v, out)
		case FLT_LOWER_S:
			n = lowerS(v, out)
		case FLT_UPPER_S:
			n = upperS(v, out)
		case FLT_LOWER_K:
			n = lowerK(v, out)
		case FLT_UPPER_K:
			n = upperK(v, out)
		case FLT_LOWER_H:
			n = lowerH(v, out)
		case FLT_UPPER_H:
			n = upperH(v, out)
		case FLT_LOWER_C:
			n = lowerC(v, out)
		case FLT_UPPER_C:
			n = upperC(v, out)
		}
		if n > 0 {
			return n
		}
	}
	return 0
}

// asciiFlags holds the filters applicable to each 1-byte character.
var asciiFlags = func() (t [0x80]int) {
	for c := 0; c < 0x80; c++ {
		if c == 0x20 { // Space
			t[c] = FLT_UPPER_S
		} else if c >= 0x30 && c <= 0x39 { // 0 - 9
			t[c] = FLT_UPPER_A | FLT_UPPER_N
		} else if c >= 0x41 && c <= 0x5a { // A - Z
			t[c] = FLT_UPPER_A | FLT_UPPER_R
		} else if c >= 0x61 && c <= 0x7a { // a - z
			t[c] = FLT_UPPER_A | FLT_UPPER_R
		} else if c >= 0x21 && c <= 0x7d && c != 0x22 && c != 0x27 && c != 0x5c {
			t[c] = FLT_UPPER_A
		}
	}
	return t
}()

func isVoiced(b []byte) bool {
	if len(b) < 6 {
		return false
	}
	if b[3] == 0xef && b[4] == 0xbe && b[5] == 0x9e {
		if (b[0] == 0xef) && (b[1] == 0xbd) && (b[2] > 0xb5) && (b[2] < 0xc0) { // ｶ - ｿ
			return true
		} else if b[0] == 0xef && b[1] == 0xbe && b[2] > 0x79 && b[2] < 0x85 { // ﾀ - ﾄ
			return true
		} else if b[0] == 0xef && b[1] == 0xbe && b[2] > 0x89 && b[2] < 0x8f { // ﾊ - ﾎ
			return true
		} else if b[0] == 0xef && b[1] == 0xbd && b[2] == 0xb3 { // ｳ
			return true
		}
	}
	return false
}

func isSemiVoiced(b []byte) bool {
	if len(b) < 6 {
		return false
	}
	if (b[3] == 0xef) && (b[4] == 0xbe) && (b[5] == 0x9f) {
		if (b[0] == 0xef) && (b[1] == 0xbe) && (b[2] > 0x89) && (b[2] < 0x8f) { // ﾊ - ﾎ
			return true
		}
	}
	return false
}

// extract returns the byte length of the multi-byte character at the head of
// s and the filters applicable to it. s[0] must be >= 0x80.
func extract(s []byte) (int, int) {
	if len(s) < 3 || s[0]&0xe0 != 0xe0 || s[1]&0x80 == 0 || s[2]&0x80 == 0 {
		if len(s) >= 2 && s[0]&0xc2 == 0xc2 && s[1]&0x80 == 0x80 {
			return 2, FLT_ASIS
		}
		return 1, FLT_ASIS
	}
	c0, c1, c2 := s[0], s[1], s[2]
	if c0 == 0xef {
		if c1 == 0xbc {
			if c2 >= 0x90 && c2 <= 0x99 { // ０ - ９
				return 3, FLT_LOWER_A | FLT_LOWER_N
			} else if c2 >= 0xa1 && c2 <= 0xba { // Ａ - Ｚ
				return 3, FLT_LOWER_A | FLT_LOWER_R
			} else if c2 != 0x82 && c2 != 0x87 && c2 != 0xbc { // except ＂ ＇ ＼
				return 3, FLT_LOWER_A
			}
		} else if c1 == 0xbd {
			if c2 >= 0x81 && c2 <= 0x9a { // ａ - ｚ
				return 3, FLT_LOWER_A | FLT_LOWER_R
			} else if c2 >= 0x80 && c2 <= 0x9d { // ｀ - ｝
				return 3, FLT_LOWER_A
			} else if c2 >= 0xa1 && c2 <= 0xbf { // ｡ ｢ ｣ ､ ･ ｦ - ｿ
				if isVoiced(s) {
					return 6, FLT_UPPER_H | FLT_UPPER_K
				}
				return 3, FLT_UPPER_H | FLT_UPPER_K
			}
		} else if c1 == 0xbe {
			if c2 >= 0x80 && c2 <= 0x84 { // ﾀ - ﾄ
				if isVoiced(s) {
					return 6, FLT_UPPER_H | FLT_UPPER_K
				}
				return 3, FLT_UPPER_H | FLT_UPPER_K
			} else if c2 >= 0x8a && c2 <= 0x8e { // ﾊ - ﾎ
				if isVoiced(s) || isSemiVoiced(s) { // voiced or semi voiced
					return 6, FLT_UPPER_H | FLT_UPPER_K
				}
				return 3, FLT_UPPER_H | FLT_UPPER_K
			} else if c2 >= 0x85 && c2 <= 0x9f { // ﾅ - ﾝﾞﾟ
				return 3, FLT_UPPER_H | FLT_UPPER_K
			}
		}
	} else if c0 == 0xe3 {
		if c1 == 0x80 {
			if c2 == 0x80 { // Space
				return 3, FLT_LOWER_S
			} else if c2 >= 0x81 && c2 <= 0x82 { // 、。
				return 3, FLT_LOWER_H | FLT_LOWER_K
			}
		} else if c1 == 0x81 {
			if c2 >= 0x81 && c2 <= 0xbf { // ぁ - み
				return 3, FLT_UPPER_C | FLT_LOWER_H
			}
		} else if c1 == 0x82 {
			if c2 >= 0x80 && c2 <= 0x93 { // む - ん
				return 3, FLT_UPPER_C | FLT_LOWER_H
			} else if c2 >= 0x9b && c2 <= 0x9c { // ゛゜
				return 3, FLT_LOWER_H | FLT_LOWER_K
			} else if c2 >= 0x9d && c2 <= 0x9e { // ゝゞ
				return 3, FLT_UPPER_C
			} else if c2 >= 0xa1 && c2 <= 0xbf { // ァ - タ
				return 3, FLT_LOWER_C | FLT_LOWER_K
			}
		} else if c1 == 0x83 {
			if c2 >= 0x80 && c2 <= 0xb3 { // チ - ン
				return 3, FLT_LOWER_C | FLT_LOWER_K
			} else if c2 == 0xb4 { // ヴ
				return 3, FLT_LOWER_K
			} else if c2 >= 0xbb && c2 <= 0xbc { // ・ー
				return 3, FLT_LOWER_H | FLT_LOWER_K
			} else if c2 >= 0xbd && c2 <= 0xbe { // ヽ ヾ
				return 3, FLT_LOWER_C
			}
		}
	}
	return 3, FLT_ASIS
}

func put1(out *[BufChars]byte, b0 byte) int {
	out[0] = b0
	return 1
}

func put3(out *[BufChars]byte, b0, b1, b2 byte) int {
	out[0], out[1], out[2] = b0, b1, b2
	return 3
}

func put6(out *[BufChars]byte, b0, b1, b2, b3, b4, b5 byte) int {
	out[0], out[1], out[2], out[3], out[4], out[5] = b0, b1, b2, b3, b4, b5
	return 6
}

func lowerR(v []byte, out *[BufChars]byte) int {
	// Ａ-Ｚ -> A-Z
	if v[2] >= 0xa1 && v[2] <= 0xba {
		return put1(out, v[2]-0x60)
	}
	// ａ-ｚ -> a-z
	if v[2] >= 0x81 && v[2] <= 0x9a {
		return put1(out, v[2]-0x20)
	}
	return 0
}

func upperR(v []byte, out *[BufChars]byte) int {
	// A-Z -> Ａ-Ｚ
	if v[0] >= 0x41 && v[0] <= 0x5a {
		return put3(out, 0xef, 0xbc, v[0]+0x60)
	}
	// a-z -> ａ-ｚ
	if v[0] >= 0x61 && v[0] <= 0x7a {
		return put3(out, 0xef, 0xbd, v[0]+0x20)
	}
	return 0
}

func lowerN(v []byte, out *[BufChars]byte) int {
	return put1(out, v[2]-0x60)
}

func upperN(v []byte, out *[BufChars]byte) int {
	return put3(out, 0xef, 0xbc, v[0]+0x60)
}

func lowerA(v []byte, out *[BufChars]byte) int {
	if v[1] == 0xbc && v[2] >= 0x81 && v[2] <= 0xbf {
		return put1(out, v[2]-0x60)
	}
	if v[1] == 0xbd && v[2] >= 0x80 && v[2] <= 0x9d {
		return put1(out, v[2]-0x20)
	}
	return 0
}

func upperA(v []byte, out *[BufChars]byte) int {
	if v[0] >= 0x21 && v[0] <= 0x5f {
		return put3(out, 0xef, 0xbc, v[0]+0x60)
	}
	if v[0] >= 0x60 && v[0] <= 0x7d {
		return put3(out, 0xef, 0xbd, v[0]+0x20)
	}
	return 0
}

func lowerS(v []byte, out *[BufChars]byte) int {
	return put1(out, 0x20)
}

func upperS(v []byte, out *[BufChars]byte) int {
	return put3(out, 0xe3, 0x80, 0x80)
}

func lowerK(v []byte, out *[BufChars]byte) int {
	var cval2 byte
	if v[1] == 0x80 {
		switch v[2] {
		case 0x81: /* 、 -> ､ */
			return put3(out, 0xef, 0xbd, 0xa4)
		case 0x82: /* 。 -> ｡ */
			return put3(out, 0xef, 0xbd, 0xa1)
		}
	} else if v[1] == 0x82 {
		switch v[2] {
		case 0x9b: /* ゛ ->  ﾞ */
			return put3(out, 0xef, 0xbe, 0x9e)
		case 0x9c: /* ゜ ->  ﾟ */
			return put3(out, 0xef, 0xbe, 0x9f)
		// ァ行: ァ -> ｧ, ィ -> ｨ, ゥ -> ｩ, ェ -> ｪ, ォ -> ｫ
		case 0xa1, 0xa3, 0xa5, 0xa7, 0xa9:
			cval2 = 0xa7 + (v[2]-0xa1)/0x02
			return put3(out, 0xef, 0xbd, cval2)
		// ア行: ア -> ｱ, イ -> ｲ, ウ -> ｳ, エ -> ｴ, オ -> ｵ
		case 0xa2, 0xa4, 0xa6, 0xa8, 0xaa:
			cval2 = 0xb1 + (v[2]-0xa2)/0x02
			return put3(out, 0xef, 0xbd, cval2)
		// カ行: カ -> ｶ, キ -> ｷ, ク -> ｸ, ケ -> ｹ, コ -> ｺ
		case 0xab, 0xad, 0xaf, 0xb1, 0xb3:
			cval2 = 0xb6 + (v[2]-0xab)/0x02
			return put3(out, 0xef, 0xbd, cval2)
		// ガ行: ガ -> ｶﾞ, ギ -> ｷﾞ, グ -> ｸﾞ, ゲ -> ｹﾞ, ゴ -> ｺﾞ
		case 0xac, 0xae, 0xb0, 0xb2, 0xb4:
			cval2 = 0xb6 + (v[2]-0xab)/0x02
			return put6(out, 0xef, 0xbd, cval2, 0xef, 0xbe, 0x9e)
		// サ行: サ -> ｻ, シ -> ｼ, ス -> ｽ, セ -> ｾ, ソ -> ｿ
		case 0xb5, 0xb7, 0xb9, 0xbb, 0xbd:
			cval2 = 0xbb + (v[2]-0xb5)/0x02
			return put3(out, 0xef, 0xbd, cval2)
		// ザ行: ザ -> ｻﾞ, ジ -> ｼﾞ, ズ -> ｽﾞ, ゼ -> ｾﾞ, ゾ -> ｿﾞ
		case 0xb6, 0xb8, 0xba, 0xbc, 0xbe:
			cval2 = 0xbb + (v[2]-0xb5)/0x02
			return put6(out, 0xef, 0xbd, cval2, 0xef, 0xbe, 0x9e)
			// タ行(タ): タ -> ﾀ
		case 0xbf:
			return put3(out, 0xef, 0xbe, 0x80)
		}
	} else if v[1] == 0x83 {
		switch v[2] {
		case 0x81:
			// タ行(チ): チ -> ﾁ
			return put3(out, 0xef, 0xbe, 0x81)
		// ダ行(ダ,ヂ): ダ -> ﾀﾞ, ヂ -> ﾁﾞ
		case 0x80, 0x82:
			cval2 = 0x80 + (v[2]-0x80)/0x02
			return put6(out, 0xef, 0xbe, cval2, 0xef, 0xbe, 0x9e)
		// タ行(ッ): ッ -> ｯ
		case 0x83:
			return put3(out, 0xef, 0xbd, 0xaf)
		// タ行(ツ,テ,ト): ツ -> ﾂ, テ -> ﾃ, ト -> ﾄ
		case 0x84, 0x86, 0x88:
			cval2 = 0x82 + (v[2]-0x84)/0x02
			return put3(out, 0xef, 0xbe, cval2)
		// ダ行(ヅ, デ, ド): ヅ -> ﾂﾞ, デ -> ﾃﾞ, ド -> ﾄﾞ
		case 0x85, 0x87, 0x89:
			cval2 = 0x82 + (v[2]-0x84)/0x02
			return put6(out, 0xef, 0xbe, cval2, 0xef, 0xbe, 0x9e)
		// ナ行: ナ -> ﾅ, ニ -> ﾆ, ヌ -> ﾇ, ネ -> ﾈ, ノ -> ﾉ
		case 0x8a, 0x8b, 0x8c, 0x8d, 0x8e:
			cval2 = v[2] - 0x05
			return put3(out, 0xef, 0xbe, cval2)
		// ハ行: ハ -> ﾊ, ヒ -> ﾋ, フ -> ﾌ, ヘ -> ﾍ, ホ -> ﾎ
		case 0x8f, 0x92, 0x95, 0x98, 0x9b:
			cval2 = 0x8a + (v[2]-0x8f)/0x03
			return put3(out, 0xef, 0xbe, cval2)
		// バ行: バ -> ﾊﾞ, ビ -> ﾋﾞ, ブ -> ﾌﾞ, ベ -> ﾍﾞ, ボ -> ﾎﾞ
		case 0x90, 0x93, 0x96, 0x99, 0x9c:
			cval2 = 0x8a + (v[2]-0x8f)/0x03
			return put6(out, 0xef, 0xbe, cval2, 0xef, 0xbe, 0x9e)
		// パ行: パ -> ﾊﾟ, ピ -> ﾋﾟ, プ -> ﾌﾟ, ペ -> ﾍﾟ, ポ -> ﾎﾟ
		case 0x91, 0x94, 0x97, 0x9a, 0x9d:
			cval2 = 0x8a + (v[2]-0x8f)/0x03
			return put6(out, 0xef, 0xbe, cval2, 0xef, 0xbe, 0x9f)
		// マ行: マ -> ﾏ, ミ -> ﾐ, ム -> ﾑ, メ -> ﾒ, モ -> ﾓ
		case 0x9e, 0x9f, 0xa0, 0xa1, 0xa2:
			cval2 = v[2] - 0x0f
			return put3(out, 0xef, 0xbe, cval2)
		// ャ行: ャ -> ｬ, ュ -> ｭ, ョ -> ｮ
		case 0xa3, 0xa5, 0xa7:
			cval2 = 0xac + (v[2]-0xa3)/0x02
			return put3(out, 0xef, 0xbd, cval2)
		// ヤ行: ヤ -> ﾔ, ユ -> ﾕ, ヨ -> ﾖ
		case 0xa4, 0xa6, 0xa8:
			cval2 = 0x94 + (v[2]-0xa4)/0x02
			return put3(out, 0xef, 0xbe, cval2)
		// ラ行: ラ -> ﾗ, リ -> ﾘ, ル -> ﾙ, レ -> ﾚ, ロ -> ﾛ
		case 0xa9, 0xaa, 0xab, 0xac, 0xad:
			cval2 = v[2] - 0x12
			return put3(out, 0xef, 0xbe, cval2)
		// ヮ -> ﾜ, ワ -> ﾜ
		case 0xae, 0xaf:
			return put3(out, 0xef, 0xbe, 0x9c)
		// ヰ -> ｲ
		case 0xb0:
			return put3(out, 0xef, 0xbd, 0xb2)
		// ヱ -> ｴ
		case 0xb1:
			return put3(out, 0xef, 0xbd, 0xb4)
		// ヲ -> ｦ
		case 0xb2:
			return put3(out, 0xef, 0xbd, 0xa6)
		// ン -> ﾝ
		case 0xb3:
			return put3(out, 0xef, 0xbe, 0x9d)
		// ヴ -> ｳﾞ
		case 0xb4:
			return put6(out, 0xef, 0xbd, 0xb3, 0xef, 0xbe, 0x9e)
		// ・ -> ･
		case 0xbb:
			return put3(out, 0xef, 0xbd, 0xa5)
		// ー -> ｰ
		case 0xbc:
			return put3(out, 0xef, 0xbd, 0xb0)
		}
	}
	return 0
}

func upperK(v []byte, out *[BufChars]byte) int {
	var cval2 byte
	if v[1] == 0xbd {
		switch v[2] {
		// ｡ -> 。
		case 0xa1:
			return put3(out, 0xe3, 0x80, 0x82)
		// ｢ -> 「, ｣ -> 」
		case 0xa2, 0xa3:
			return put3(out, 0xe3, 0x80, v[2]-0x16)
		// ､ -> 、
		case 0xa4:
			return put3(out, 0xe3, 0x80, 0x81)
		// ･ -> ・
		case 0xa5:
			return put3(out, 0xe3, 0x83, 0xbb)
		// ｦ -> ヲ
		case 0xa6:
			return put3(out, 0xe3, 0x83, 0xb2)
		// ｧ行: ｧ -> ァ, ｨ -> ィ, ｩ -> ゥ, ｪ -> ェ, ｫ -> ォ
		case 0xa7, 0xa8, 0xa9, 0xaa, 0xab:
			cval2 = 0xa1 + (v[2]-0xa7)*0x02
			return put3(out, 0xe3, 0x82, cval2)
		// ｬ行: ｬ -> ャ, ｭ -> ュ, ｮ -> ョ
		case 0xac, 0xad, 0xae:
			cval2 = 0xa3 + (v[2]-0xac)*0x02
			return put3(out, 0xe3, 0x83, cval2)
		// ｯ -> ッ
		case 0xaf:
			return put3(out, 0xe3, 0x83, 0x83)
		// ｰ -> ー
		case 0xb0:
			return put3(out, 0xe3, 0x83, 0xbc)
		// ｱ行: ｱ -> ア, ｲ -> イ, ｴ -> エ, ｵ -> オ
		case 0xb1, 0xb2, 0xb4, 0xb5:
			cval2 = 0xa2 + (v[2]-0xb1)*0x02
			return put3(out, 0xe3, 0x82, cval2)
		// ｱ行(ｳ,ｳﾞ): ｳ -> ウ, ｳﾞ -> ヴ
		case 0xb3:
			if len(v) > 5 && v[5] == 0x9e {
				return put3(out, 0xe3, 0x83, 0xb4)
			} else {
				return put3(out, 0xe3, 0x82, 0xa6)
			}
		// ｶ行: ｶ ->カ, ｷ -> キ, ｸ -> ク, ｹ -> ケ, ｺ -> コ
		// ｶﾞ行: ｶﾞ -> ガ, ｷﾞ -> ギ, ｸﾞ -> グ, ｹﾞ -> ゲ, ｺﾞ -> ゴ
		case 0xb6, 0xb7, 0xb8, 0xb9, 0xba:
			if len(v) > 5 && v[5] == 0x9e {
				cval2 = 0xac + (v[2]-0xb6)*0x02
			} else {
				cval2 = 0xab + (v[2]-0xb6)*0x02
			}
			return put3(out, 0xe3, 0x82, cval2)
		// ｻ行: ｻ -> サ, ｼ -> シ, ｽ -> ス, ｾ -> セ, ｿ -> ソ
		// ｻﾞ行: ｻﾞ -> ザ, ｼﾞ -> ジ, ｽﾞ -> ズ, ｾﾞ -> ゼ, ｿﾞ -> ゾ
		case 0xbb, 0xbc, 0xbd, 0xbe, 0xbf:
			if len(v) > 5 && v[5] == 0x9e {
				cval2 = 0xb6 + (v[2]-0xbb)*0x02
			} else {
				cval2 = 0xb5 + (v[2]-0xbb)*0x02
			}
			return put3(out, 0xe3, 0x82, cval2)
		}
	} else if v[1] == 0xbe {
		switch v[2] {
		// タ行(タ)・ダ行(ダ): ﾀ -> タ, ﾀﾞ -> ダ
		case 0x80:
			if len(v) > 5 && v[5] == 0x9e {
				return put3(out, 0xe3, 0x83, 0x80)
			} else {
				return put3(out, 0xe3, 0x82, 0xbf)
			}
		// タ行(チ)・ダ行(ヂ): ﾁ -> チ, ﾁﾞ -> ヂ
		case 0x81:
			if len(v) > 5 && v[5] == 0x9e {
				cval2 = 0x82
			} else {
				cval2 = 0x81
			}
			return put3(out, 0xe3, 0x83, cval2)
		// タ行(ツ,テ,ト)・ダ行(ヅ,デ,ド): ﾃ -> テ, ﾄ -> ト, ﾃﾞ -> デ, ﾄﾞ -> ド
		case 0x82, 0x83, 0x84:
			if len(v) > 5 && v[5] == 0x9e {
				cval2 = 0x85 + (v[2]-0x82)*0x02
			} else {
				cval2 = 0x84 + (v[2]-0x82)*0x02
			}
			return put3(out, 0xe3, 0x83, cval2)
		// ナ行: (ﾅ -> ナ, ﾆ -> ニ, ﾇ -> ヌ, ﾈ -> ネ, ﾉ -> ノ)
		case 0x85, 0x86, 0x87, 0x88, 0x89:
			cval2 = v[2] + 0x05
			return put3(out, 0xe3, 0x83, cval2)
		// ハ行: ﾊ -> ハ, ﾋ -> ヒ, ﾌ -> フ, ﾍ -> ヘ, ﾎ -> ホ
		// バ行: ﾊﾞ -> バ, ﾋﾞ -> ビ, ﾌﾞ -> ブ, ﾍﾞ -> ベ, ﾎﾞ -> ボ
		// パ行: ﾊﾟ -> パ, ﾋﾟ -> ピ, ﾌﾟ -> プ, ﾍﾟ -> ペ, ﾎﾟ -> ポ,
		case 0x8a, 0x8b, 0x8c, 0x8d, 0x8e:
			if len(v) > 5 && v[5] == 0x9e {
				cval2 = 0x8a + (v[2]-0x88)*0x03
			} else if len(v) > 5 && v[5] == 0x9f {
				cval2 = 0x8b + (v[2]-0x88)*0x03
			} else {
				cval2 = 0x89 + (v[2]-0x88)*0x03
			}
			return put3(out, 0xe3, 0x83, cval2)
		// マ行: ﾏ -> マ, ﾐ -> ミ, ﾑ -> ム, ﾒ -> メ, ﾓ -> モ
		case 0x8f, 0x90, 0x91, 0x92, 0x93:
			cval2 = v[2] + 0x0f
			return put3(out, 0xe3, 0x83, cval2)
		// ヤ行: ﾔ -> ヤ, ﾕ -> ユ, ﾖ -> ヨ
		case 0x94, 0x95, 0x96:
			cval2 = 0xa4 + (v[2]-0x94)*0x02
			return put3(out, 0xe3, 0x83, cval2)
		case 0x97, 0x98, 0x99, 0x9a, 0x9b:
			cval2 = v[2] + 0x12
			return put3(out, 0xe3, 0x83, cval2)
		// ﾜ -> ワ
		case 0x9c:
			return put3(out, 0xe3, 0x83, 0xaf)
		// ﾝ -> ン
		case 0x9d:
			return put3(out, 0xe3, 0x83, 0xb3)
		case 0x9e: /* ﾞ */
			return put3(out, 0xe3, 0x82, 0x9b)
		case 0x9f: /* ﾟ */
			return put3(out, 0xe3, 0x82, 0x9c)
		}
	}
	return 0
}

func lowerH(v []byte, out *[BufChars]byte) int {
	var cval2 byte
	if v[1] == 0x80 {
		switch v[2] {
		// 、 -> ､
		case 0x81:
			return put3(out, 0xef, 0xbd, 0xa4)
		// 。 -> ｡
		case 0x82:
			return put3(out, 0xef, 0xbd, 0xa1)
		}
	} else if v[1] == 0x81 {
		switch v[2] {
		// ぁ行: ぁ -> ｧ, ぃ -> ｨ, ぅ -> ｩ, ぇ -> ｪ, ぉ -> ｫ
		case 0x81, 0x83, 0x85, 0x87, 0x89:
			cval2 := 0xa7 + (v[2]-0x81)/0x02
			return put3(out, 0xef, 0xbd, cval2)
		// あ行: あ -> ｱ, い -> ｲ, う -> ｳ, え -> ｴ, お -> ｵ
		case 0x82, 0x84, 0x86, 0x88, 0x8a:
			cval2 := 0xb1 + (v[2]-0x82)/0x02
			return put3(out, 0xef, 0xbd, cval2)
		// か行: か -> ｶ, き -> ｷ, く -> ｸ, け -> ｹ, こ -> ｺ
		case 0x8b, 0x8d, 0x8f, 0x91, 0x93:
			cval2 := 0xb6 + (v[2]-0x8b)/0x02
			return put3(out, 0xef, 0xbd, cval2)
		// が行: が -> ｶﾞ, ぎ -> ｷﾞ, ぐ -> ｸﾞ, げ -> ｹﾞ, ご -> ｺﾞ
		case 0x8c, 0x8e, 0x90, 0x92, 0x94:
			cval2 = 0xb6 + (v[2]-0x8c)/0x02
			return put6(out, 0xef, 0xbd, cval2, 0xef, 0xbe, 0x9e)
		// さ行: さ -> ｻ, し -> ｼ, す -> ｽ, せ -> ｾ, そ -> ｿ
		case 0x95, 0x97, 0x99, 0x9b, 0x9d:
			cval2 = 0xbb + (v[2]-0x95)/0x02
			return put3(out, 0xef, 0xbd, cval2)
		// ざ行: ざ -> ｻﾞ, じ -> ｼﾞ, ず -> ｽﾞ, ぜ -> ｾﾞ, ぞ -> ｿﾞ
		case 0x96, 0x98, 0x9a, 0x9c, 0x9e:
			cval2 = 0xbb + (v[2]-0x96)/0x02
			return put6(out, 0xef, 0xbd, cval2, 0xef, 0xbe, 0x9e)
		// た行(た): た -> ﾀ
		case 0x9f:
			return put3(out, 0xef, 0xbe, 0x80)
		case 0xa1:
			// た行(ち): ち -> ﾁ
			return put3(out, 0xef, 0xbe, 0x81)
		// だ行(だ,ぢ): だ -> ﾀﾞ, ぢ -> ﾁﾞ
		case 0xa0, 0xa2:
			cval2 = 0x80 + (v[2]-0xa0)/0x02
			return put6(out, 0xef, 0xbe, cval2, 0xef, 0xbe, 0x9e)
		// た行(っ): っ -> ｯ
		case 0xa3:
			return put3(out, 0xef, 0xbd, 0xaf)
		// た行(つ,て,と): つ -> ﾂ, て -> ﾃ, と -> ﾄ
		case 0xa4, 0xa6, 0xa8:
			cval2 = 0x82 + (v[2]-0xa4)/0x02
			return put3(out, 0xef, 0xbe, cval2)
		// だ行(づ,で,ど): づ -> ﾂﾞ, で -> ﾃﾞ, ど -> ﾄﾞ
		case 0xa5, 0xa7, 0xa9:
			cval2 = 0x82 + (v[2]-0xa4)/0x02
			return put6(out, 0xef, 0xbe, cval2, 0xef, 0xbe, 0x9e)
		// な行: な -> ﾅ, に -> ﾆ, ぬ -> ﾇ, ね -> ﾈ, の -> ﾉ
		case 0xaa, 0xab, 0xac, 0xad, 0xae:
			cval2 = v[2] - 0x25
			return put3(out, 0xef, 0xbe, cval2)

		// は行: は -> ﾊ, ひ -> ﾋ, ふ -> ﾌ, へ -> ﾍ, ほ -> ﾎ
		case 0xaf, 0xb2, 0xb5, 0xb8, 0xbb:
			cval2 = 0x8a + (v[2]-0xaf)/0x03
			return put3(out, 0xef, 0xbe, cval2)
			// ば行: ば -> ﾊﾞ, び -> ﾋﾞ, ぶ -> ﾌﾞ, べ -> ﾍﾞ, ぼ -> ﾎﾞ
		case 0xb0, 0xb3, 0xb6, 0xb9, 0xbc:
			cval2 = 0x8a + (v[2]-0xaf)/0x03
			return put6(out, 0xef, 0xbe, cval2, 0xef, 0xbe, 0x9e)
		// ぱ行: ぱ -> ﾊﾟ, ぴ -> ﾋﾟ, ぷ -> ﾌﾟ, ぺ -> ﾍﾟ, ぽ -> ﾎﾟ
		case 0xb1, 0xb4, 0xb7, 0xba, 0xbd:
			cval2 = 0x8a + (v[2]-0xaf)/0x03
			return put6(out, 0xef, 0xbe, cval2, 0xef, 0xbe, 0x9f)
		// ま行(ま,み): ま -> ﾏ, み -> ﾐ
		case 0xbe, 0xbf:
			cval2 = v[2] - 0x2f
			return put3(out, 0xef, 0xbe, cval2)
		}
	} else if v[1] == 0x82 {
		switch v[2] {
		// ま行(む,め,も): む -> ﾑ, め -> ﾒ, も -> ﾓ
		case 0x80, 0x81, 0x82:
			cval2 = v[2] + 0x11
			return put3(out, 0xef, 0xbe, cval2)
		// ゃ行: ゃ -> ｬ, ゅ -> ｭ, ょ -> ｮ
		case 0x83, 0x85, 0x87:
			cval2 = 0xac + (v[2]-0x83)/0x02
			return put3(out, 0xef, 0xbd, cval2)
		// や行: や -> ﾔ, ゆ -> ﾕ, よ -> ﾖ
		case 0x84, 0x86, 0x88:
			cval2 = 0x94 + (v[2]-0x84)/0x02
			return put3(out, 0xef, 0xbe, cval2)
		// ら行: ら -> ﾗ, り -> ﾘ, る -> ﾙ, れ -> ﾚ, ろ -> ﾛ
		case 0x89, 0x8a, 0x8b, 0x8c, 0x8d:
			cval2 = v[2] + 0x0e
			return put3(out, 0xef, 0xbe, cval2)
		// ゎ -> ﾜ, わ -> ﾜ
		case 0x8e, 0x8f:
			return put3(out, 0xef, 0xbe, 0x9c)
		// ゐ -> ｲ
		case 0x90:
			return put3(out, 0xef, 0xbd, 0xb2)
		// ゑ -> ｴ
		case 0x91:
			return put3(out, 0xef, 0xbd, 0xb4)
		// を -> ｦ
		case 0x92:
			return put3(out, 0xef, 0xbd, 0xa6)
		// ン -> ﾝ
		case 0x93:
			return put3(out, 0xef, 0xbe, 0x9d)
		// ゛ -> ﾞ
		case 0x9b:
			return put3(out, 0xef, 0xbe, 0x9e)
		// ゜ -> ﾟ
		case 0x9c:
			return put3(out, 0xef, 0xbe, 0x9f)

		}
	} else if v[1] == 0x83 {
		switch v[2] {
		// ・ -> ･
		case 0xbb:
			return put3(out, 0xef, 0xbd, 0xa5)
		// ー -> ｰ
		case 0xbc:
			return put3(out, 0xef, 0xbd, 0xb0)
		}
	}
	return 0
}

func upperH(v []byte, out *[BufChars]byte) int {
	var cval2 byte
	if v[1] == 0xbd {
		switch v[2] {
		// ｡ -> 。
		case 0xa1:
			return put3(out, 0xe3, 0x80, 0x82)
		// ｢ -> 「, ｣ -> 」
		case 0xa2, 0xa3:
			cval2 = v[2] - 0x16
			return put3(out, 0xe3, 0x80, cval2)
		// ､ -> 、
		case 0xa4:
			return put3(out, 0xe3, 0x80, 0x81)
		// ･ -> ・
		case 0xa5:
			return put3(out, 0xe3, 0x83, 0xbb)
		// ｦ -> を
		case 0xa6:
			return put3(out, 0xe3, 0x82, 0x92)
		// ｧ行: ｧ -> ぁ, ｨ -> ぃ, ｩ -> ぅ, ｪ -> ぇ, ｫ -> ぉ
		case 0xa7, 0xa8, 0xa9, 0xaa, 0xab:
			cval2 = 0x81 + (v[2]-0xa7)*0x02
			return put3(out, 0xe3, 0x81, cval2)
		// ｬ行: ｬ -> ゃ, ｭ -> ゅ, ｮ -> ょ
		case 0xac, 0xad, 0xae:
			cval2 = 0x83 + (v[2]-0xac)*0x02
			return put3(out, 0xe3, 0x82, cval2)
		// ﾀ行(ｯ): ｯ -> っ
		case 0xaf:
			return put3(out, 0xe3, 0x81, 0xa3)
		// ｰ -> ー
		case 0xb0: /* ｰ */
			return put3(out, 0xe3, 0x83, 0xbc)
		// ｱ行: ｱ -> あ, ｲ -> い, ｳ -> う, ｴ -> え, ｵ -> お
		case 0xb1, 0xb2, 0xb4, 0xb5:
			cval2 = 0x82 + (v[2]-0xb1)*0x02
			return put3(out, 0xe3, 0x81, cval2)
		case 0xb3:
			if len(v) > 5 && v[5] == 0x9e {
				return put6(out, 0xe3, 0x81, 0x86, 0xe3, 0x82, 0x9b)
			}
			return put3(out, 0xe3, 0x81, 0x86)
		// ｶ行: ｶ -> か, ｷ -> き, ｸ -> く, ｹ -> け, ｺ -> こ
		// ｶﾞ行: ｶﾞ -> が, ｷﾞ -> ぎ, ｸﾞ -> ぐ, ｹﾞ -> げ, ｺﾞ -> ご
		case 0xb6, 0xb7, 0xb8, 0xb9, 0xba:
			if len(v) > 5 && v[5] == 0x9e {
				cval2 = 0x8c + (v[2]-0xb6)*0x02
			} else {
				cval2 = 0x8b + (v[2]-0xb6)*0x02
			}
			return put3(out, 0xe3, 0x81, cval2)
		// ｻ行: ｻ -> さ, ｼ -> し, ｽ -> す, ｾ -> せ, ｿ -> そ
		// ｻﾞ行: ｻﾞ -> ざ, ｼﾞ -> じ, ｽﾞ -> ず, ｾﾞ -> ぜ, ｿﾞ -> ぞ
		case 0xbb, 0xbc, 0xbd, 0xbe, 0xbf:
			if len(v) > 5 && v[5] == 0x9e {
				cval2 = 0x96 + (v[2]-0xbb)*0x02
			} else {
				cval2 = 0x95 + (v[2]-0xbb)*0x02
			}
			return put3(out, 0xe3, 0x81, cval2)
		}
	} else if v[1] == 0xbe {
		switch v[2] {
		// ﾀ行(ﾀ,ﾁ): ﾀ -> た, ﾁ -> ち
		// ﾀﾞ行(ﾀﾞ,ﾁﾞ): ﾀﾞ -> だ, ﾁﾞ -> ぢ
		case 0x80, 0x81:
			if len(v) > 5 && v[5] == 0x9e {
				cval2 = 0xa0 + (v[2]-0x80)*0x02
			} else {
				cval2 = 0x9f + (v[2]-0x80)*0x02
			}
			return put3(out, 0xe3, 0x81, cval2)
		// ﾀ行(ﾂ,ﾃ,ﾄ): ﾂ -> つ, ﾃ -> て, ﾄ -> と
		// ﾀﾞ行(ﾂﾞ,ﾃﾞ,ﾄﾞ): ﾂﾞ -> づ, ﾃﾞ -> で, ﾄﾞ -> ど
		case 0x82, 0x83, 0x84:
			if len(v) > 5 && v[5] == 0x9e {
				cval2 = 0xa5 + (v[2]-0x82)*0x02
			} else {
				cval2 = 0xa4 + (v[2]-0x82)*0x02
			}
			return put3(out, 0xe3, 0x81, cval2)
		// ﾅ行: ﾅ -> な, ﾆ -> に, ﾇ -> ぬ, ﾈ -> ね, ﾉ -> の
		case 0x85, 0x86, 0x87, 0x88, 0x89:
			cval2 = v[2] + 0x25
			return put3(out, 0xe3, 0x81, cval2)
			// ﾊ行: ﾊ -> は, ﾋ -> ひ, ﾌ -> ふ, ﾍ -> へ, ﾎ -> ほ
			// ﾊﾞ行: ﾊﾞ -> ば, ﾋﾞ -> び, ﾌﾞ -> ぶ, ﾍﾞ -> べ, ﾎﾞ -> ぼ
			// ﾊﾟ行: ﾊﾟ -> ぱ, ﾋﾟ -> ぴ, ﾌﾟ -> ぷ, ﾍﾟ -> ぺ, ﾎﾟ -> ぽ
		case 0x8a, 0x8b, 0x8c, 0x8d, 0x8e:
			if len(v) > 5 && v[5] == 0x9e {
				cval2 = 0xb0 + (v[2]-0x8a)*0x03
			} else if len(v) > 5 && v[5] == 0x9f {
				cval2 = 0xb1 + (v[2]-0x8a)*0x03
			} else {
				cval2 = 0xaf + (v[2]-0x8a)*0x03
			}
			return put3(out, 0xe3, 0x81, cval2)
		// ﾏ行: ﾏ -> ま, ﾐ -> み
		case 0x8f, 0x90:
			cval2 = v[2] + 0x2f
			return put3(out, 0xe3, 0x81, cval2)
		// ﾏ行: ﾑ -> む, ﾒ -> め, ﾓ -> も
		case 0x91, 0x92, 0x93:
			cval2 = v[2] - 0x11
			return put3(out, 0xe3, 0x82, cval2)
		// ﾔ行: ﾔ -> や, ﾕ -> ゆ, ﾖ -> よ
		case 0x94, 0x95, 0x96:
			cval2 = 0x84 + (v[2]-0x94)*0x02
			return put3(out, 0xe3, 0x82, cval2)
		// ﾗ行: ﾗ -> ら, ﾘ -> り, ﾙ -> る, ﾚ -> れ, ﾛ -> ろ
		case 0x97, 0x98, 0x99, 0x9a, 0x9b:
			cval2 = v[2] - 0x0e
			return put3(out, 0xe3, 0x82, cval2)
		// ﾜ行: ﾜ -> わ
		case 0x9c:
			return put3(out, 0xe3, 0x82, 0x8f)
		// ﾝ -> ん
		case 0x9d:
			return put3(out, 0xe3, 0x82, 0x93)
		// ﾞ -> ゛
		case 0x9e:
			return put3(out, 0xe3, 0x82, 0x9b)
		// ﾟ -> ゜
		case 0x9f:
			return put3(out, 0xe3, 0x82, 0x9c)
		}
	}
	return 0
}

func lowerC(v []byte, out *[BufChars]byte) int {
	switch v[1] {
	case 0x82: // ァ - タ
		if v[2] >= 0xa1 && v[2] <= 0xbf {
			return put3(out, 0xe3, 0x81, v[2]-0x20)
		}
	case 0x83:
		if v[2] >= 0x80 && v[2] <= 0x9f { // ダ - ミ
			return put3(out, 0xe3, 0x81, v[2]+0x20)
		} else if v[2] >= 0xa0 && v[2] <= 0xb3 { // ム - ン
			return put3(out, 0xe3, 0x82, v[2]-0x20)
		} else if v[2] >= 0xbd && v[2] <= 0xbe { // ヽヾ
			return put3(out, 0xe3, 0x82, v[2]-0x20)
		}
	}
	return 0
}

func upperC(v []byte, out *[BufChars]byte) int {
	switch v[1] {
	case 0x81:
		if v[2] >= 0x81 && v[2] <= 0x9f { // ぁ - た
			return put3(out, 0xe3, 0x82, v[2]+0x20)
		} else if v[2] >= 0xa0 && v[2] <= 0xbf { // だ - み
			return put3(out, 0xe3, 0x83, v[2]-0x20)
		}
	case 0x82:
		if v[2] >= 0x80 && v[2] <= 0x93 { // む - ん
			return put3(out, 0xe3, 0x83, v[2]+0x20)
		} else if v[2] >= 0x9d && v[2] <= 0x9e { // ゝゞ
			return put3(out, 0xe3, 0x83, v[2]+0x20)
		}
	}
	return 0
}
