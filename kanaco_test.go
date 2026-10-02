package kanaco

import (
	"bytes"
	"fmt"
	"io"
	"os"
	"path/filepath"
	"strings"
	"testing"
)

var (
	output = "output.*.txt"
)

func mode4Test(path string) string {
	basename := filepath.Base(path)
	ext := filepath.Ext(basename)
	tmp := strings.Split(strings.ReplaceAll(strings.ReplaceAll(basename, "output.", ""), ext, ""), ".")
	mode := strings.Builder{}
	for _, m := range tmp {
		switch m {
		case "la":
			mode.WriteString("a")
		case "lc":
			mode.WriteString("c")
		case "lh":
			mode.WriteString("h")
		case "lk":
			mode.WriteString("k")
		case "ln":
			mode.WriteString("n")
		case "lr":
			mode.WriteString("r")
		case "ls":
			mode.WriteString("s")
		case "ua":
			mode.WriteString("A")
		case "uc":
			mode.WriteString("C")
		case "uh":
			mode.WriteString("H")
		case "uk":
			mode.WriteString("K")
		case "un":
			mode.WriteString("N")
		case "ur":
			mode.WriteString("R")
		case "us":
			mode.WriteString("S")
		}
	}
	return mode.String()
}

func TestByte(t *testing.T) {
	content, err := os.ReadFile("./data/input.txt")
	if err != nil {
		t.Error(err.Error())
	}
	paths, err := filepath.Glob("./data/" + output)
	if err != nil {
		t.Error(err.Error())
	}
	for _, path := range paths {
		mode := mode4Test(path)
		expect, err := os.ReadFile(path)
		if err != nil {
			t.Error(err.Error())
		}
		result := Byte(content, mode)
		if !bytes.Equal(result, expect) {
			expects := bytes.Split(expect, []byte("\n"))
			results := bytes.Split(result, []byte("\n"))
			msg := strings.Builder{}
			msg.WriteString(fmt.Sprintf("\n[%s] ---------\n", mode))
			for k, e := range expects {
				var r []byte
				if k < len(results) {
					r = results[k]
				}
				if !bytes.Equal(r, e) {
					msg.WriteString(fmt.Sprintf("Expect(%d): ", k))
					msg.Write(e)
					msg.WriteString(fmt.Sprintf("\nResult(%d): ", k))
					msg.Write(r)
					msg.WriteString("\n")
				}
			}
			t.Error(msg.String())
		}
	}
}

func TestString(t *testing.T) {
	content, err := os.ReadFile("./data/input.txt")
	if err != nil {
		t.Error(err.Error())
	}
	paths, err := filepath.Glob("./data/" + output)
	if err != nil {
		t.Error(err.Error())
	}
	for _, path := range paths {
		mode := mode4Test(path)
		expect, err := os.ReadFile(path)
		if err != nil {
			t.Error(err.Error())
		}
		result := String(string(content), mode)
		if strings.Compare(result, string(expect)) != 0 {
			fmt.Printf("[%s]----\n%s\n", mode, result)
			expects := bytes.Split(expect, []byte("\n"))
			results := strings.Split(result, "\n")
			msg := strings.Builder{}
			msg.WriteString(fmt.Sprintf("\n[%s] ---------\n", mode))
			for k, e := range expects {
				r := results[k]
				if strings.Compare(string(r), string(e)) != 0 {
					msg.WriteString(fmt.Sprintf("Expect(%d): ", k+1))
					msg.Write(e)
					msg.WriteString(fmt.Sprintf("\nResult(%d): ", k+1))
					msg.WriteString(r)
					msg.WriteString("\n")
				}
			}
			t.Error(msg.String())
		}
	}
}

func TestNewReader(t *testing.T) {
	f, _ := os.Open("./data/input.txt")
	r := NewReader(f, "a")
	tp := fmt.Sprintf("%T", r)
	if tp != "*kanaco.Reader" {
		t.Errorf("Reader is invalid (%s)", tp)
	}
}

func readAll(t *testing.T, r *Reader) []byte {
	t.Helper()
	results := []byte{}
	buf := make([]byte, 4096)
	for {
		n, err := r.Read(buf)
		if err == io.EOF {
			return results
		}
		if err != nil {
			t.Fatal(err.Error())
		}
		results = append(results, buf[:n]...)
	}
}

func TestRead(t *testing.T) {
	content, err := os.ReadFile("./data/input.txt")
	if err != nil {
		t.Fatal(err.Error())
	}
	paths, err := filepath.Glob("./data/" + output)
	if err != nil {
		t.Fatal(err.Error())
	}
	if len(paths) == 0 {
		t.Fatal("no output files")
	}
	for _, path := range paths {
		mode := mode4Test(path)
		expect, err := os.ReadFile(path)
		if err != nil {
			t.Fatal(err.Error())
		}
		// The data files end with a newline. Also read the input without it,
		// so that a last line ending at EOF is checked as well.
		for _, trim := range []bool{false, true} {
			in, exp := content, expect
			if trim {
				in = bytes.TrimSuffix(in, []byte("\n"))
				exp = bytes.TrimSuffix(exp, []byte("\n"))
			}
			results := readAll(t, NewReader(bytes.NewReader(in), mode))
			if !bytes.Equal(results, exp) {
				rLines := bytes.Split(results, []byte("\n"))
				eLines := bytes.Split(exp, []byte("\n"))
				msg := strings.Builder{}
				msg.WriteString(fmt.Sprintf("\n[%s] (trailing newline removed: %t) ---------\n", mode, trim))
				for k, et := range eLines {
					var rs []byte
					if k < len(rLines) {
						rs = rLines[k]
					}
					if !bytes.Equal(et, rs) {
						msg.WriteString(fmt.Sprintf("Expect(%d): ", k+1))
						msg.Write(et)
						msg.WriteString(fmt.Sprintf("\nResult(%d): ", k+1))
						msg.Write(rs)
						msg.WriteString("\n")
					}
				}
				t.Error(msg.String())
			}
		}
	}
}

func TestReadLastLineWithoutNewline(t *testing.T) {
	results := readAll(t, NewReader(strings.NewReader("ｱｲｳ\nｶﾞｷﾞ"), "H"))
	if expect := "あいう\nがぎ"; string(results) != expect {
		t.Errorf("Expect: %q, Result: %q", expect, results)
	}
}
