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
	// paths, err := filepath.Glob("./data/" + output)
	paths, err := filepath.Glob("./data/output.H.txt")
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
				r := results[k]
				if strings.Compare(string(r), string(e)) != 0 {
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

func TestRead(t *testing.T) {
	paths, _ := filepath.Glob("./data/" + output)
	for _, path := range paths {
		expects, _ := os.ReadFile(path)
		mode := mode4Test(path)
		f, err := os.Open("./data/input.txt")
		if err != nil {
			t.Fatal(err.Error())
		}
		r := NewReader(f, mode)
		results := []byte{}
		buf := make([]byte, 4096)
		for {
			n, err := r.Read(buf)
			if err == io.EOF {
				break
			}
			if err != nil {
				t.Error(err.Error())
				break
			}
			results = append(results, buf[:n]...)
		}
		f.Close()
		if !bytes.Equal(results, expects) {
			rLines := bytes.Split(results, []byte("\n"))
			eLines := bytes.Split(expects, []byte("\n"))
			msg := strings.Builder{}
			msg.WriteString(fmt.Sprintf("\n[%s] ---------\n", mode))
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

func TestReadLastLineWithoutNewline(t *testing.T) {
	r := NewReader(strings.NewReader("ｱｲｳ\nｶﾞｷﾞ"), "H")
	buf := make([]byte, 64)
	results := []byte{}
	for {
		n, err := r.Read(buf)
		if err == io.EOF {
			break
		}
		if err != nil {
			t.Fatal(err.Error())
		}
		results = append(results, buf[:n]...)
	}
	if expect := "あいう\nがぎ"; string(results) != expect {
		t.Errorf("Expect: %q, Result: %q", expect, results)
	}
}
