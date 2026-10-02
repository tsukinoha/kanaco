from libc.stdlib cimport free

cdef extern from "kanaco.h":
    char *kanaco_convert(const char *s, size_t length, const char *mode,
                         size_t mode_len, size_t *out_len) nogil

def conv(s, str m) -> str:
    cdef bytes b, mb
    cdef const char *src
    cdef const char *msrc
    cdef size_t src_len, mode_len, out_len = 0
    cdef char *tmp = NULL

    t = type(s)
    if t is str:
        b = s.encode("utf-8")
    elif t is int:
        b = str(s).encode("utf-8")
    elif t is bytes:
        b = s
    else:
        raise TypeError("Invalid Data Type.")
    mb = m.encode("utf-8")

    # Pass byte lengths (not character counts) and use the returned length,
    # so multi-byte text and embedded NUL bytes survive the round trip.
    src, src_len = b, len(b)
    msrc, mode_len = mb, len(mb)
    with nogil:
        tmp = kanaco_convert(src, src_len, msrc, mode_len, &out_len)
    if tmp == NULL:
        raise MemoryError()
    try:
        return tmp[:out_len].decode("utf-8", errors="ignore")
    finally:
        free(tmp)
