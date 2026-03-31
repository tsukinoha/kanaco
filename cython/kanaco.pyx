from libc.stdlib cimport free

cdef extern from "kanaco.h":
    char *convert(char *s, int length, char *mode, int mode_len)

def conv(s, str m) -> str:
    cdef char *c = NULL
    cdef str ret
    cdef bytes b, mb
    try:
        if isinstance(s, str):
            b = <bytes>s.encode("utf-8")
            mb = <bytes>m.encode("utf-8")
        elif isinstance(s, bytes):
            b = <bytes>s
            mb = <bytes>m.encode("utf-8")
        elif "__str__" in dir(s):
            b = <bytes>(str(s).encode("utf-8"))
            mb = <bytes>(m.encode("utf-8"))
        else:
            raise TypeError(f"unsupported type \"{type(s)}\"")
        c = convert(b, len(b), mb, len(mb))
        ret = c.decode("utf-8", errors="ignore")
    finally:
        free(c)
    return ret
