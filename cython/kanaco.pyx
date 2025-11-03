from libc.stdlib cimport free

cdef extern from "kanaco.h":
    char *convert(char *s, int length, char *mode, int mode_len)

def conv(s, m: str) -> str:
    cdef bytes bs, bm
    cdef const char *cs = NULL
    cdef const char *cm = NULL
    cdef char *tmp = NULL
    cdef str ret

    # check data type for s
    if isinstance(s, str):
        bs = s.encode("utf-8")
    elif isinstance(s, (int, float)):
        bs = str(s).encode("utf-8")
    elif isinstance(s, bytes):
        bs = s
    else:
        raise TypeError("Invalid data type for s")
    # check data type for m
    if not isinstance(m, str):
        raise TypeError("mode must be a string")
    bm = m.encode("utf-8")

    cs = bs
    cm = bm

    try:
        tmp = convert(cs, len(bs), cm, len(bm))
        if tmp == NULL:
            raise MemoryError("convert error")
        ret = tmp.decode("utf-8", errors="ignore")
    finally:
        if tmp != NULL:
            free(tmp)

    return ret
